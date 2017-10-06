#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include "operation_id.h"

class HTMLDocument;

class BenchmarkFixtureHTMLBuilder
{
public:
    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;

    typedef std::unordered_map<OperationId, OutputDurationType> BenchmarkFixtureResultForOperation;

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

    static HTMLDocument Build(const std::vector<BenchmarkFixtureResultForPlatform>& results, const char* fileName);
};
