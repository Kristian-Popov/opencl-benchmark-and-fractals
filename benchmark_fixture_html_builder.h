#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "operation_step.h"
#include "html_document.h"

class BenchmarkFixtureHTMLBuilder
{
public:
    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;

    typedef std::unordered_map<OperationStep, OutputDurationType> BenchmarkFixtureResultForOperation;

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

    BenchmarkFixtureHTMLBuilder( const char* fileName );

    void AddFixtureResults( const BenchmarkFixtureResultForFixture& results );
    std::shared_ptr<HTMLDocument> GetHTMLDocument()
    {
        return document_;
    }
private:
    std::shared_ptr<HTMLDocument> document_;
};
