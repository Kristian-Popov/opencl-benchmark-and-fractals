#include "indicators/duration_indicator.h"

#include "fixtures/fixture_family.h"
#include <utils.h>

namespace
{
    const char* const kTotalDuration = "Total duration";
}

void DurationIndicator::Calculate( const BenchmarkResultForFixtureFamily& benchmark )
{
    for( auto& fixture_data: benchmark.benchmark )
    {
        const BenchmarkResultForFixture& fixture_results = fixture_data.second;
        FixtureCalculatedData d;
        if( !fixture_results.durations.empty() )
        {
            // Calculate duration for every step and total one.
            Duration total_duration;

            for( auto& iter_results : fixture_results.durations )
            {
                for( auto& step_results : iter_results )
                {
                    d.step_durations[step_results.first] += step_results.second;
                    total_duration += step_results.second;
                    {
                        auto iter = d.step_min_durations.find( step_results.first );
                        if( iter == d.step_min_durations.end() )
                        {
                            iter = d.step_min_durations.emplace( step_results.first, Duration::Max() ).first;
                        }
                        if( step_results.second < iter->second )
                        {
                            iter->second = step_results.second;
                        }
                    }
                    {
                        auto iter = d.step_max_durations.find( step_results.first );
                        if( iter == d.step_max_durations.end() )
                        {
                            iter = d.step_max_durations.emplace( step_results.first, Duration::Min() ).first;
                        }
                        if( step_results.second > iter->second )
                        {
                            iter->second = step_results.second;
                        }
                    }
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
        for( auto& step_data: fixture_data.second.step_durations )
        {
            const std::string step_name = OperationStepDescriptionRepository::GetSerializeId( step_data.first );
            serialized_fixture_data[step_name]["average"] = step_data.second;
            serialized_fixture_data[step_name]["min"] = fixture_data.second.step_min_durations.at( step_data.first );
            serialized_fixture_data[step_name]["max"] = fixture_data.second.step_max_durations.at( step_data.first );
        }
        result[fixture_data.first.Serialize()] = serialized_fixture_data;
    }
    return result;
}
