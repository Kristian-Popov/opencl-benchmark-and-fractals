#pragma once

#include <memory>
#include <vector>

#include "fixture.h"
#include "benchmark_reporter_interface.h"

class FixtureRunner
{
public:
    struct FixturesToRun
    {
        bool trivialFactorial = true;
        bool dampedWave2D = true;
        bool kochCurve = true;
        bool multibrotSet = true;
    };

    void Run( std::unique_ptr<BenchmarkReporter> timeWriter, FixturesToRun fixturesToRun );
private:
    void Clear();
    void SetFloatingPointEnvironment();
    void CreateTrivialFixtures();
    void CreateDampedWave2DFixtures();
    void CreateKochCurveFixtures();
    void CreateMultibrotSetFixtures();
    void FillContextsMap();

    std::unordered_map<cl_device_id, boost::compute::context> contexts_;
    std::vector<std::shared_ptr<Fixture>> fixtures_;
};