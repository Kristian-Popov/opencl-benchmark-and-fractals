#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include "devices/platform_list.h"
#include "reporters/benchmark_results.h"

class BenchmarkReporter
{
public:
    virtual void AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results ) = 0;

    virtual void Initialize( const PlatformList& platform_list )
    {}

    /*
        Optional method to flush all contents to output
    */
    virtual void Flush() {}

    virtual ~BenchmarkReporter() noexcept {}
};
