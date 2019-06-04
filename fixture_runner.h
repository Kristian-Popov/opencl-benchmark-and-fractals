#ifndef KPV_FIXTURE_RUNNER_H_
#define KPV_FIXTURE_RUNNER_H_

#include <algorithm>
#include <boost/algorithm/clamp.hpp>
#include <boost/log/trivial.hpp>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "data_verification_failed_exception.h"
#include "devices/opencl_device.h"
#include "devices/platform_list.h"
#include "fixture_registry.h"
#include "fixtures/fixture.h"
#include "fixtures/fixture_family.h"
#include "program_build_failed_exception.h"
#include "reporters/json_benchmark_reporter.h"
#include "run_settings.h"
#include "utils/duration.h"
#include "utils/utils.h"

namespace kpv {

// TODO forbid construction outside allowed context if possible
class FixtureRunner {
public:
    void Run(RunSettings settings) {
        BOOST_LOG_TRIVIAL(info) << "Welcome to OpenCL benchmark.";

        EXCEPTION_ASSERT(settings.min_iterations >= 1);
        EXCEPTION_ASSERT(settings.max_iterations >= 1);

        std::vector<std::string> present_categories =
            FixtureRegistry::instance().GetAllCategories();
        std::sort(present_categories.begin(), present_categories.end());
        std::sort(settings.category_list.begin(), settings.category_list.end());

        std::unordered_set<std::string> categories_to_run;

        // Build a list of categories that are mentioned in command line but don't exist
        std::vector<std::string> missing_categories;
        std::set_difference(
            settings.category_list.cbegin(), settings.category_list.cend(),
            present_categories.cbegin(), present_categories.cend(),
            std::back_inserter(missing_categories));
        if (!missing_categories.empty()) {
            BOOST_LOG_TRIVIAL(warning)
                << "Fixture categories \"" << Utils::VectorToString(missing_categories)
                << "\" are in include/exclude list but do not exist.";
        }

        // Build a list of categories to run
        if (settings.operation == RunSettings::kRunAllExcept) {
            std::set_difference(
                present_categories.cbegin(), present_categories.cend(),
                settings.category_list.cbegin(), settings.category_list.cend(),
                std::inserter(categories_to_run, categories_to_run.begin()));
        } else if (settings.operation == RunSettings::kRunOnly) {
            std::set_intersection(
                present_categories.cbegin(), present_categories.cend(),
                settings.category_list.cbegin(), settings.category_list.cend(),
                std::inserter(categories_to_run, categories_to_run.begin()));
        } else if (settings.operation == RunSettings::kList) {
            // List all categories
            BOOST_LOG_TRIVIAL(info) << "List of categories" << std::endl
                                    << Utils::VectorToString(present_categories);
            return;
        } else {
            BOOST_LOG_TRIVIAL(error) << "Selected fixture filter method is not supported. Exiting.";
            return;
        }

        JsonBenchmarkReporter reporter(settings.output_file_name);
        PlatformList platform_list(settings.device_config);
        reporter.Initialize(platform_list);

        BOOST_LOG_TRIVIAL(info) << "We have " << categories_to_run.size()
                                << " fixture categories to run";

        int family_index = 1;  // Used for logging only
        for (const auto& p : FixtureRegistry::instance()) {
            if (categories_to_run.count(p.first) == 0) {
                // This fixture is in exclude list, skip it
                BOOST_LOG_TRIVIAL(info) << "Skipping category " << p.first;
                continue;
            }

            std::shared_ptr<FixtureFamily> fixture_family = p.second(platform_list);
            std::string fixture_name = fixture_family->name;
            FixtureFamilyBenchmark results;
            results.fixture_family = fixture_family;

            BOOST_LOG_TRIVIAL(info) << "Starting fixture family \"" << fixture_name << "\"";

            for (auto& fixture_data : fixture_family->fixtures) {
                const FixtureId& fixture_id = fixture_data.first;
                std::shared_ptr<Fixture>& fixture = fixture_data.second;
                FixtureBenchmark fixture_results;

                BOOST_LOG_TRIVIAL(info)
                    << "Starting run on device \"" << fixture_id.device()->Name() << "\"";

                try {
                    std::vector<std::string> required_extensions = fixture->GetRequiredExtensions();
                    std::sort(required_extensions.begin(), required_extensions.end());

                    std::vector<std::string> have_extensions = fixture->Device()->Extensions();
                    std::sort(have_extensions.begin(), have_extensions.end());

                    std::vector<std::string> missed_extensions;
                    std::set_difference(
                        required_extensions.cbegin(), required_extensions.cend(),
                        have_extensions.cbegin(), have_extensions.cend(),
                        std::back_inserter(missed_extensions));
                    if (!missed_extensions.empty()) {
                        fixture_results.failure_reason = "Required extension(s) are not available";
                        // Destroy fixture to release some memory sooner
                        fixture.reset();

                        BOOST_LOG_TRIVIAL(warning)
                            << "Device \"" << fixture_id.device()->Name()
                            << "\" doesn't support extensions needed for fixture: "
                            << Utils::VectorToString(missed_extensions);

                        results.benchmark.insert(std::make_pair(fixture_id, fixture_results));

                        continue;
                    }

                    fixture->Initialize();  // TODO move higher when fixture is constructed, may be
                                            // disable altogether?

                    std::vector<std::unordered_map<std::string, Duration>> durations;

                    // Warm-up for one iteration to get estimation of execution time
                    Fixture::RuntimeParams params;
                    params.additional_params = settings.additional_params;
                    std::unordered_map<std::string, Duration> warmup_result =
                        fixture->Execute(params);
                    durations.push_back(warmup_result);
                    Duration total_operation_duration = std::accumulate(
                        warmup_result.begin(), warmup_result.end(), Duration(),
                        [](Duration acc, const std::pair<std::string, Duration>& r) {
                            return acc + r.second;
                        });
                    uint64_t iteration_count_long =
                        settings.target_execution_time / total_operation_duration;
                    EXCEPTION_ASSERT(iteration_count_long < std::numeric_limits<int>::max());
                    int iteration_count = static_cast<int>(iteration_count_long);
                    iteration_count =
                        boost::algorithm::clamp(
                            iteration_count, settings.min_iterations, settings.max_iterations) -
                        1;
                    EXCEPTION_ASSERT(iteration_count >= 0);

                    if (settings.verify_results) {
                        fixture->VerifyResults();
                    }

                    if (settings.store_results) {
                        fixture->StoreResults();
                    }

                    for (int i = 0; i < iteration_count; ++i) {
                        durations.push_back(fixture->Execute(params));
                    }
                    fixture_results.durations = durations;
                } catch (ProgramBuildFailedException& e) {
                    BOOST_LOG_TRIVIAL(error)
                        << "Program for fixture \"" << fixture_name
                        << "\" failed to build on device \"" << e.DeviceName() << "\"";
                    BOOST_LOG_TRIVIAL(info) << "Build options: " << e.BuildOptions();
                    BOOST_LOG_TRIVIAL(info) << "Build log: " << std::endl << e.BuildLog();
                    BOOST_LOG_TRIVIAL(debug) << e.what();
                    fixture_results.failure_reason = "OpenCL Program failed to build";
                } catch (boost::compute::opencl_error& e) {
                    BOOST_LOG_TRIVIAL(error) << "OpenCL error occured: " << e.what();
                    fixture_results.failure_reason = e.what();
                } catch (DataVerificationFailedException& e) {
                    BOOST_LOG_TRIVIAL(error) << "Data verification failed: " << e.what();
                    fixture_results.failure_reason = "Data verification failed";
                } catch (std::exception& e) {
                    BOOST_LOG_TRIVIAL(error) << "Exception occured: " << e.what();
                    fixture_results.failure_reason = e.what();
                }

                // Destroy fixture to release some memory sooner
                fixture.reset();

                BOOST_LOG_TRIVIAL(info)
                    << "Finished run on device \"" << fixture_id.device()->Name() << "\"";

                results.benchmark.insert(std::make_pair(fixture_id, fixture_results));
            }

            reporter.AddFixtureFamilyResults(results);

            BOOST_LOG_TRIVIAL(info)
                << "Fixture family \"" << fixture_name << "\" finished successfully.";
            ++family_index;
        }
        reporter.Flush();

        BOOST_LOG_TRIVIAL(info) << "Done";
    }
};
}  // namespace kpv

#endif  // KPV_FIXTURE_RUNNER_H_
