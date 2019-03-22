#pragma once

#include <memory>
#include <vector>

#include "devices/platform_list.h"
#include "utils/duration.h"
#include "fixtures/fixture.h"
#include "fixtures/fixture_family.h"
#include "reporters/benchmark_reporter.h"

class FixtureRunner
{
public:
    struct FixturesToRun
    {
        bool trivialFactorial = true;
        bool dampedWave2D = true;
        bool kochCurve = true;
        bool multibrotSet = true;
        bool multiprecisionFactorial = true;
    };

    struct RunSettings
    {
        FixturesToRun fixturesToRun;
        int minIterations = 1;
        int maxIterations = std::numeric_limits<int>::max();
        Duration targetFixtureExecutionTime;
        bool verifyResults = true;
        bool storeResults = true;
        bool debug_mode = false;
    };

    void Run( std::unique_ptr<BenchmarkReporter> timeWriter, RunSettings settings );
private:
    void Clear();
    void SetFloatingPointEnvironment();
    void CreateTrivialFixtures();
    template<typename T, typename D = std::normal_distribution<T>>
    void CreateDampedWave2DFixtures();
    template<typename T, typename T4>
    void CreateKochCurveFixtures();
    template<typename T, typename P>
    void FixtureRunner::CreateMultibrotSetFixtures();
    //void CreateMultiprecisionFactorialFixtures();

    std::vector<std::shared_ptr<FixtureFamily>> fixture_families_;
    PlatformList platform_list_;
};