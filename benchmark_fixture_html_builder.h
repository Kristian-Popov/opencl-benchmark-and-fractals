#pragma once

#include <chrono>
#include <string>
#include <vector>

class HTMLDocument;

class BenchmarkFixtureHTMLBuilder
{
public:
    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;

    struct BenchmarkFixtureResultForOperation
    {
        std::string operationDescription;
        OutputDurationType duration;
    };

    struct BenchmarkFixtureResultForDevice
    {
        std::string deviceName;
        std::vector<BenchmarkFixtureResultForOperation> perOperationResults;
    };

    struct BenchmarkFixtureResultForPlatform
    {
        std::string platformName;
        std::vector<BenchmarkFixtureResultForDevice> perDeviceResults;
    };

    static HTMLDocument Build(const std::vector<BenchmarkFixtureResultForPlatform>& results, const char* fileName);
};
