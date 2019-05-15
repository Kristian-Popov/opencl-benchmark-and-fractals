#pragma once

#include <memory>

#include "fixture_runner.h"
#include "reporters/json_benchmark_reporter.h"

class FixtureCommandLineProcessor {
public:
    static std::unique_ptr<JsonBenchmarkReporter> Process(
        int argc, char** argv, FixtureRunner::RunSettings& settings);

private:
    static void PrintVersion();
};
