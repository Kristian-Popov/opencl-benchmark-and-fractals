#pragma once

#include <memory>

#include "reporters/json_benchmark_reporter.h"
#include "fixture_runner.h"

class FixtureCommandLineProcessor
{
public:
    static std::unique_ptr<JsonBenchmarkReporter> Process(
        int argc, char** argv, FixtureRunner::RunSettings& settings
    );
private:
    static void PrintVersion();
};