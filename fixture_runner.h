#pragma once

#include <memory>
#include <vector>

#include "devices/platform_list.h"
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
        DurationType targetFixtureExecutionTime = std::chrono::milliseconds( 100 );
        bool verifyResults = true;
        bool storeResults = true;
    };

    void Run( std::unique_ptr<BenchmarkReporter> timeWriter, RunSettings settings );
private:
    void Clear();
    void SetFloatingPointEnvironment();
    void CreateTrivialFixtures();
    void CreateDampedWave2DFixtures();
    void CreateKochCurveFixtures();
    void CreateMultibrotSetFixtures();
    void CreateMultiprecisionFactorialFixtures();

    std::vector<std::shared_ptr<FixtureFamily>> fixture_families_;
    PlatformList platform_list_;
};