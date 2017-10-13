#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include "operation_step.h"

class BenchmarkTimeWriterInterface
{
public:
    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;
    // Vector of iterations, every iteration has a map of operation steps and a corresponding duration
    typedef std::vector<std::unordered_map<OperationStep, OutputDurationType>> BenchmarkFixtureResultForOperation;

    struct BenchmarkFixtureResultForDevice
    {
        std::string deviceName;
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
        std::vector<BenchmarkFixtureResultForPlatform> perFixtureResults;
    };

    virtual void WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results ) = 0;

    /*
        Optional method to flush all contents to output
    */
    virtual void Flush() {}
};