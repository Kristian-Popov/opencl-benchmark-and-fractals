#ifndef KPV_COMMAND_LINE_PROCESSOR_H_
#define KPV_COMMAND_LINE_PROCESSOR_H_

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

#include "fixture_runner.h"

namespace kpv {
namespace CommandLineProcessor {
void PrintVersion() { BOOST_LOG_TRIVIAL(info) << "Version is 0.1.0"; }
// Returns true if execution should continue
bool Process(int argc, char** argv, RunSettings& settings) {
    namespace log = boost::log::trivial;
    namespace po = boost::program_options;

    static const char* const kDefaultOutputFileName = "output.json";
    static const char* const kDefaultTargetTime = "100ms";
    static const std::unordered_map<std::string /* suffix */, double /* multiplier */>
        kTimeMultipliers = {{"ns", 1e-9}, {"mcs", 1e-6}, {"ms", 1e-3}, {"s", 1}};
    /*
    Picked mostly randomly, IMO should be something like 1e9 (may be somebody wants
    1ns fixture to run for 1 second)
    */
    const int max_iterations_cap = 1000000000;

    std::string category_list;
    int min_iterations = 1;
    int max_iterations = max_iterations_cap;
    std::string target_time;
    std::string additional_params;
    std::string devices;

    boost::program_options::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("verbose,v", "verbose mode (more output is written to the console)")
        ("version,V", "print version")
        ("output-file,o", po::value<std::string>(&settings.output_file_name)->default_value(kDefaultOutputFileName),
            "name and path to output JSON file")
        ("list", "list all fixture categories")
        ("run-all-except", po::value<std::string>(&category_list),
            "run all available fixtures except specified ones (IDs separated by a comma)")
        ("run-only", po::value<std::string>(&category_list),
            "run only specified fixtures (IDs separated by a comma)")
        ("min-iterations,m", po::value<int>(&min_iterations)->default_value(1),
            "minimum number of iterations")
        ("max-iterations,M", po::value<int>(&max_iterations)->default_value(max_iterations_cap),
            "maximum number of iterations")
        ("target-time,t", po::value<std::string>(&target_time)->default_value(kDefaultTargetTime),
            "target execution time for one fixture (examples: 100ms, 1.5ns, 9s)")
        ("additional-params", po::value<std::string>(&additional_params),
            "additional parameters that are passed to fixtures")
        ("host,h", "run fixtures on host CPU (without involving OpenCL)")
        ("cpu,c", "run fixtures on OpenCL CPU devices")
        ("gpu,g", "run fixtures on OpenCL GPU devices")
        ("other-devices", "run fixtures on OpenCL accelerators and other devices")
        ;
    // clang-format on

    po::variables_map vm;
    try {
        boost::program_options::store(
            boost::program_options::parse_command_line(argc, argv, desc), vm);
    } catch (boost::program_options::invalid_command_line_syntax& e) {
        BOOST_LOG_TRIVIAL(fatal) << "Wrong command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL(fatal) << desc;
        throw;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(fatal) << "Caught exception when parsing command line arguments: "
                                 << e.what();
        BOOST_LOG_TRIVIAL(fatal) << desc;
        throw;
    }
    boost::program_options::notify(vm);

    if (vm.count("help") > 0) {
        BOOST_LOG_TRIVIAL(fatal) << desc;
        return false;
    }

    if (vm.count("version") > 0) {
        PrintVersion();
        return false;
    }

    if (min_iterations < 1) {
        BOOST_LOG_TRIVIAL(fatal) << "Minimum number of iterations cannot be less than 1";
        return false;
    }
    if (max_iterations < min_iterations) {
        BOOST_LOG_TRIVIAL(fatal) << "Maximum number of iterations is less than minimum number";
        return false;
    }
    settings.min_iterations = min_iterations;
    settings.max_iterations = max_iterations;
    try {
        size_t index = 0;
        double val = std::stod(target_time, &index);
        double multiplier = kTimeMultipliers.at(target_time.substr(index));
        settings.target_execution_time = Duration(std::chrono::duration<double>(val * multiplier));
    } catch (std::exception&) {
        BOOST_LOG_TRIVIAL(fatal) << "Incorrect format of target execution time";
        return false;
    }

    const bool list = vm.count("list") > 0;
    const bool run_all_except = vm.count("run-all-except") > 0;
    const bool run_only = vm.count("run-only") > 0;
    if ((list && run_all_except) || (run_all_except && run_only) || (list && run_only)) {
        BOOST_LOG_TRIVIAL(fatal) << "More than one operation command is given";
        return false;
    }

    settings.operation = RunSettings::kRunAllExcept;  // Default mode
    if (list) {
        settings.operation = RunSettings::kList;
    } else if (run_all_except) {
        settings.operation = RunSettings::kRunAllExcept;
    } else if (run_only) {
        settings.operation = RunSettings::kRunOnly;
    }

    boost::char_separator<char> comma(",");
    boost::tokenizer<boost::char_separator<char> > category_tokenizer(category_list, comma);
    std::copy(
        category_tokenizer.begin(), category_tokenizer.end(),
        std::back_inserter(settings.category_list));

    log::severity_level min_severity = (vm.count("verbose") > 0) ? log::trace : log::info;
    boost::log::core::get()->set_filter(log::severity >= min_severity);

    // If no device options are given, enable all of them. Otherwise disable all and then go
    // type by type
    if ((vm.count("cpu") > 0) || (vm.count("gpu") > 0) || (vm.count("host") > 0) ||
        (vm.count("other-devices") > 0)) {
        // Disable all devices by default
        settings.device_config = DeviceConfiguration(false);
        if (vm.count("cpu") > 0) {
            settings.device_config.cpu_opencl_devices = true;
        }
        if (vm.count("gpu") > 0) {
            settings.device_config.gpu_opencl_devices = true;
        }
        if (vm.count("host") > 0) {
            settings.device_config.host_device = true;
        }
        if (vm.count("other-devices") > 0) {
            settings.device_config.other_opencl_devices = true;
        }
    }

    settings.additional_params = additional_params;
    return true;
}
}  // namespace CommandLineProcessor
};  // namespace kpv

#endif  // KPV_COMMAND_LINE_PROCESSOR_H_
