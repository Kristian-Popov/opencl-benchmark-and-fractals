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
        bool kochCurve = true;
        bool multibrotSet = true;
    };

    static void Run( std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter, 
        FixturesToRun fixturesToRun );
private:
    static void CreateTrivialFixtures( std::vector<std::shared_ptr<Fixture>>& fixtures );
    static void CreateDampedWave2DFixtures( 
        std::vector<std::shared_ptr<Fixture>>& fixtures,
        std::vector<std::shared_ptr<FixtureThatReturnsData<long double>>>& fixturesWithData );
    static void CreateKochCurveFixtures( 
        std::vector<std::shared_ptr<Fixture>>& fixtures,
        std::vector<std::shared_ptr<FixtureThatReturnsData<long double>>>& fixturesWithData,
        std::vector<std::shared_ptr<FixtureThatReturnsData<long double>>>& fixtureToWriteResultToSVG );
    static void CreateMultibrotSetFixtures(
        std::vector<std::shared_ptr<Fixture>>& fixtures,
        std::vector<std::shared_ptr<FixtureThatReturnsData<cl_ushort4>>>& fixturesToWriteDataToPNG );

    static std::vector<boost::compute::device> FillDevicesList();
    static std::unordered_map<cl_device_id, boost::compute::context> FillContextsMap(
        const std::vector<boost::compute::device>& devices );
};