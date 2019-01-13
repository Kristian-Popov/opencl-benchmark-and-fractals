#include "failure_indicator.h"

void FailureIndicator::Calculate( const BenchmarkResultForFixtureFamily& benchmark )
{
    for( auto& data : benchmark.benchmark )
    {
        if( data.second.failure_reason )
        {
            failures_.insert( std::make_pair( data.first, data.second.failure_reason.value() ) );
        }
    }
}

nlohmann::json FailureIndicator::SerializeValue()
{
    nlohmann::json result;
    for( auto& data : failures_ )
    {
        result[data.first.Serialize()]["description"] = data.second;
    }
    return result;
}