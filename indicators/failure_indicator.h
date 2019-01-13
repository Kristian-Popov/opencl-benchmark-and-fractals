#pragma once

#include "indicators/indicator_interface.h"
#include "reporters/benchmark_results.h"

class FailureIndicator : public IndicatorInterface
{
public:
    explicit FailureIndicator( const BenchmarkResultForFixtureFamily& benchmark )
    {
        Calculate( benchmark );
    }

    nlohmann::json SerializeValue() override;
private:
    void Calculate( const BenchmarkResultForFixtureFamily& benchmark );
    std::unordered_map<FixtureId, std::string> failures_;
};