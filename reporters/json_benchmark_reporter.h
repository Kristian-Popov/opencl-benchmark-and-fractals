#pragma once

#include "nlohmann/json.hpp"
#include "reporters/benchmark_reporter.h"

class PlatformList;

class JsonBenchmarkReporter : public BenchmarkReporter {
public:
    JsonBenchmarkReporter(const std::string& file_name);

    void Initialize(const PlatformList& platform_list) override;

    void AddFixtureFamilyResults(const FixtureFamilyBenchmark& results) override;

    /*
    Optional method to flush all contents to output
    */
    void Flush() override;

private:
    std::string file_name_;
    bool pretty_ = true;  // TODO make configurable?
    nlohmann::json tree_;
};
