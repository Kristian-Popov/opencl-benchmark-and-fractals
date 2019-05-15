#include "reporters/json_benchmark_reporter.h"

#include <time.h>

#include <boost/log/trivial.hpp>
#include <fstream>
#include <iomanip>

#include "fixtures/fixture_family.h"
#include "indicators/duration_indicator.h"

namespace {
std::string GetCurrentTimeString() {
    // TODO replace with some library?
    // Based on https://stackoverflow.com/a/10467633
    time_t now = time(nullptr);
    struct tm tstruct;
    char buf[80];
    // TODO gmtime is not thread-safe, do something with that?
    tstruct = *gmtime(&now);
    strftime(buf, sizeof(buf), "%FT%TZ", &tstruct);
    return buf;
}
}  // namespace

JsonBenchmarkReporter::JsonBenchmarkReporter(const std::string& file_name)
    : file_name_(file_name) {}

void JsonBenchmarkReporter::Initialize(const PlatformList& platform_list) {
    tree_["baseInfo"] = {{"about", "This file was built by OpenCL benchmark."},
                         {"time", GetCurrentTimeString()},
                         {"formatVersion", "0.1.0"}};
    for (auto& platform : platform_list.AllPlatforms()) {
        nlohmann::json devices = nlohmann::json::array();
        for (auto& device : platform->GetDevices()) {
            devices.push_back(device->UniqueName());
        }
        tree_["deviceList"][platform->Name()] = devices;
    }
    tree_["fixtureFamilies"] = json::array();
}

void JsonBenchmarkReporter::AddFixtureFamilyResults(const FixtureFamilyBenchmark& results) {
    using nlohmann::json;

    json fixture_family_tree = {{"name", results.fixture_family->name}};
    if (results.fixture_family->element_count) {
        fixture_family_tree["elementCount"] = results.fixture_family->element_count.value();
    }

    json fixture_tree = json::array();
    for (auto& data : results.benchmark) {
        DurationIndicator indicator{data.second};
        json current_fixture_tree = json::object({{"name", data.first.Serialize()}});

        if (!indicator.IsEmpty()) {
            indicator.SerializeValue(current_fixture_tree["duration"]);
        }
        if (data.second.failure_reason) {
            current_fixture_tree["failureReason"] = data.second.failure_reason.value();
        }
        fixture_tree.push_back(current_fixture_tree);
    }

    fixture_family_tree["fixtures"] = fixture_tree;
    tree_["fixtureFamilies"].push_back(fixture_family_tree);
}

void JsonBenchmarkReporter::Flush() {
    try {
        std::ofstream o(file_name_);
        o.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
        if (pretty_) {
            o << std::setw(4);
        }
        o << tree_ << std::endl;
    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(error)
            << "Caught exception when building report in JSON format and writing to a file "
            << file_name_ << ": " << e.what();
        throw;
    }
}
