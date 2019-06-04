#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include "devices/platform_list.h"
#include "reporters/benchmark_results.h"

namespace kpv {
class BenchmarkReporter {
public:
    virtual void AddFixtureFamilyResults(const FixtureFamilyBenchmark& results) = 0;

    virtual void Initialize(const PlatformList& platform_list) {}

    /*
    Optional method to flush all contents to output
    */
    virtual void Flush() {}

    virtual ~BenchmarkReporter() noexcept {}
};
}  // namespace kpv
