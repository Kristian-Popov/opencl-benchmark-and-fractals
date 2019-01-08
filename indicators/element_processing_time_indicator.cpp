#include "indicators/element_processing_time_indicator.h"

#include "fixtures/fixture_family.h"
#include "indicators/duration_indicator.h"
#include <utils.h>

namespace
{
    const char* const kFailureReason = "Failure reason";
    const char* const kTotalDuration = "Total duration";
}

void ElementProcessingTimeIndicator::Calculate( const BenchmarkResultForFixtureFamily& benchmark )
{
    DurationIndicator temp_indicator( benchmark );
    std::unordered_map<FixtureId, DurationIndicator::FixtureCalculatedData> durations = temp_indicator.GetCalculatedData();

    for( auto& fixture_data : durations )
    {
        const auto& fixture_results = fixture_data.second;
        FixtureCalculatedData d;
        if( fixture_results.failure_reason )
        {
            d.failure_reason = fixture_results.failure_reason;
        }
        else
        {
            boost::optional<int32_t> element_count = benchmark.fixture_family->element_count;
            EXCEPTION_ASSERT( element_count );
            d.total_duration = fixture_results.total_duration / element_count.value();
            for( auto& step_data: fixture_results.step_durations )
            {
                d.step_durations.emplace( step_data.first, step_data.second / element_count.value() );
            }
        }
        calculated_.insert( std::make_pair( fixture_data.first, d ) );
    }
}

nlohmann::json ElementProcessingTimeIndicator::SerializeValue()
{
    // TODO serialize operation_steps_ and fixture_family_name_?
    nlohmann::json result;
    for( auto& fixture_data: calculated_ )
    {
        nlohmann::json serialized_fixture_data;
        if( fixture_data.second.failure_reason )
        {
            serialized_fixture_data[kFailureReason] = fixture_data.second.failure_reason.value();
        }
        else
        {
            serialized_fixture_data[kTotalDuration] = fixture_data.second.total_duration;
            for( auto& step_data: fixture_data.second.step_durations )
            {
                serialized_fixture_data[OperationStepDescriptionRepository::GetSerializeId( step_data.first )] =
                    step_data.second;
            }
        }
        result[fixture_data.first.Serialize()] = serialized_fixture_data;
    }
    return result;
}
