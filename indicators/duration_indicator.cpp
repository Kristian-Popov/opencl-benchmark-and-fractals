#include "indicators/duration_indicator.h"

#include "fixtures/fixture_family.h"
#include <utils.h>

namespace
{
    const char* const kTotalDuration = "Total duration";
}

void DurationIndicator::Calculate( const BenchmarkResultForFixtureFamily& benchmark )
{
    operation_steps_ = benchmark.fixture_family->operation_steps;

    for( auto& fixture_data: benchmark.benchmark )
    {
        const BenchmarkResultForFixture& fixture_results = fixture_data.second;
        FixtureCalculatedData d;
        if( !fixture_results.durations.empty() )
        {
            // Calculate duration for every step and total one.
            Duration total_duration;
            for( int stepIndex = 0; stepIndex < operation_steps_.size(); ++stepIndex )
            {
                OperationStep step = operation_steps_.at( stepIndex );

                for( auto& results: fixture_results.durations )
                {
                    auto range = results.equal_range( step );
                    Duration val = std::accumulate( range.first, range.second, Duration(), []
                        ( Duration lhs, const std::pair<OperationStep, Duration>& rhs ) -> Duration
                        {
                            return lhs + rhs.second;
                        }
                    );
                    d.step_durations[step] += val;
                    total_duration += val;
                }
            }

            // Divide durations by amount of iterations
            std::size_t element_count = fixture_results.durations.size();
            total_duration /= element_count;
            for( auto& p: d.step_durations )
            {
                p.second /= element_count;
            }

            d.total_duration = total_duration;
        }

        // Store data only if total duration is larger than zero
        if( d.total_duration != Duration() )
        {
            calculated_.insert( std::make_pair( fixture_data.first, d ) );
        }
    }
}

nlohmann::json DurationIndicator::SerializeValue()
{
    nlohmann::json result;
    for( auto& fixture_data: calculated_ )
    {
        nlohmann::json serialized_fixture_data;
        serialized_fixture_data[kTotalDuration] = fixture_data.second.total_duration;
        for( OperationStep step: operation_steps_ )
        {
            serialized_fixture_data[OperationStepDescriptionRepository::GetSerializeId( step )] =
                fixture_data.second.step_durations[step];
        }
        result[fixture_data.first.Serialize()] = serialized_fixture_data;
    }
    return result;
}
