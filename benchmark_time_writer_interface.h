#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/optional.hpp>

#include "operation_step.h"

class BenchmarkTimeWriterInterface
{
public:
    typedef long double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType> OutputDurationType;
    // Vector of iterations, every iteration has a map of operation steps and a corresponding duration
    typedef std::vector<std::unordered_map<OperationStep, OutputDurationType>> BenchmarkFixtureResultForOperation;

    struct BenchmarkFixtureResultForDevice
    {
        std::string deviceName;
        boost::optional<std::string> failureReason;
        BenchmarkFixtureResultForOperation perOperationResults;
    };

    struct BenchmarkFixtureResultForPlatform
    {
        std::string platformName;
        std::vector<BenchmarkFixtureResultForDevice> perDeviceResults;
    };

    struct BenchmarkFixtureResultForFixture
    {
        std::string fixtureName;
        std::vector<OperationStep> operationSteps;
        boost::optional<size_t> elementsCount;
        std::vector<BenchmarkFixtureResultForPlatform> perFixtureResults;
    };

    virtual void WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results ) = 0;

    /*
        Optional method to flush all contents to output
    */
    virtual void Flush() {}
};