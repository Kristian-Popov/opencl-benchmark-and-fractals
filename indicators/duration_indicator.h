#ifndef KPV_DURATION_INDICATOR_H_
#define KPV_DURATION_INDICATOR_H_

#include <boost/optional.hpp>
#include <unordered_map>

#include "nlohmann/json.hpp"
#include "reporters/benchmark_results.h"
#include "utils/duration.h"

class DurationIndicator {
public:
    explicit DurationIndicator(const FixtureBenchmark& benchmark) { Calculate(benchmark); }

    void SerializeValue(nlohmann::json& tree) {
        tree["iterationCount"] = calculated_.iteration_count;
        if (calculated_.iteration_count == 1) {
            // We have a single duration
            for (auto& step_data : calculated_.step_durations) {
                tree["duration"][step_data.first] = step_data.second;
            }
        } else if (calculated_.iteration_count > 1) {
            // We have a duration range
            for (auto& step_data : calculated_.step_durations) {
                tree["duration"][step_data.first]["avg"] = step_data.second;
                tree["duration"][step_data.first]["min"] =
                    calculated_.step_min_durations.at(step_data.first);
                tree["duration"][step_data.first]["max"] =
                    calculated_.step_max_durations.at(step_data.first);
            }
        }
    }

    bool IsEmpty() const { return calculated_.total_duration == Duration(); }

private:
    struct FixtureCalculatedData {
        std::unordered_map<std::string, Duration> step_durations;
        std::unordered_map<std::string, Duration> step_min_durations;
        std::unordered_map<std::string, Duration> step_max_durations;
        std::size_t iteration_count;
        // Is not serialized, is just a temporary solution to check if duration is empty
        // TODO check in some better way?
        Duration total_duration;
    };

    void Calculate(const FixtureBenchmark& benchmark) {
        if (!benchmark.durations.empty()) {
            // Calculate duration for every step and total one.
            Duration total_duration;

            for (auto& iter_results : benchmark.durations) {
                for (auto& step_results : iter_results) {
                    calculated_.step_durations[step_results.first] += step_results.second;
                    total_duration += step_results.second;
                    {
                        auto iter = calculated_.step_min_durations.find(step_results.first);
                        if (iter == calculated_.step_min_durations.end()) {
                            iter = calculated_.step_min_durations
                                       .emplace(step_results.first, Duration::Max())
                                       .first;
                        }
                        if (step_results.second < iter->second) {
                            iter->second = step_results.second;
                        }
                    }
                    {
                        auto iter = calculated_.step_max_durations.find(step_results.first);
                        if (iter == calculated_.step_max_durations.end()) {
                            iter = calculated_.step_max_durations
                                       .emplace(step_results.first, Duration::Min())
                                       .first;
                        }
                        if (step_results.second > iter->second) {
                            iter->second = step_results.second;
                        }
                    }
                }
            }

            // Divide durations by amount of iterations
            calculated_.iteration_count = benchmark.durations.size();
            total_duration /= calculated_.iteration_count;
            for (auto& p : calculated_.step_durations) {
                p.second /= calculated_.iteration_count;
            }

            calculated_.total_duration = total_duration;
        }
    }

    FixtureCalculatedData calculated_;
};

#endif  // KPV_DURATION_INDICATOR_H_
