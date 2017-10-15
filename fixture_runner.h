#pragma once

#include <memory>
#include <vector>

#include "fixture_that_returns_data.h"
#include "benchmark_time_writer_interface.h"

class FixtureRunner
{
public:
    struct FixturesToRun
    {
        bool trivialFactorial = true;
        bool dampedWave2D = true;
    };

    static void Run( std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter, 
        FixturesToRun fixturesToRun );
private:
    static void CreateTrivialFixtures( std::vector<std::shared_ptr<Fixture>>& fixtures );
    static void CreateDampedWave2DFixtures( 
        std::vector<std::shared_ptr<Fixture>>& fixtures,
        std::vector<std::shared_ptr<FixtureThatReturnsData<cl_float>>>& fixturesWithData );
};