#include "indicators/throughput_indicator.h"

#include "fixtures/fixture_family.h"
#include "indicators/duration_indicator.h"
#include <utils.h>

namespace
{
    const char* const kThroughput = "Throughput";
}

void ThroughputIndicator::Calculate( const BenchmarkResultForFixtureFamily& benchmark )
{
    DurationIndicator temp_indicator( benchmark );
    std::unordered_map<FixtureId, DurationIndicator::FixtureCalculatedData> durations = temp_indicator.GetCalculatedData();

    for( auto& fixture_data: durations )
    {
        const auto& fixture_results = fixture_data.second;
        if( fixture_results.total_duration != Duration() ) // Calculate data only if duration is not zero
        {
            FixtureCalculatedData d;

            using namespace std::literals::chrono_literals;
            boost::optional<int32_t> element_count = benchmark.fixture_family->element_count;
            EXCEPTION_ASSERT( element_count );
            double throughput_double = element_count.value() * ( Duration( 1s ) / fixture_results.total_duration );
            EXCEPTION_ASSERT( throughput_double > 0 );
            EXCEPTION_ASSERT( throughput_double < std::numeric_limits<int32_t>::max() );
            d.throughput = static_cast<int32_t>( throughput_double );
            calculated_.insert( std::make_pair( fixture_data.first, d ) );
        }
    }
}

nlohmann::json ThroughputIndicator::SerializeValue()
{
    nlohmann::json result;
    for( auto& fixture_data: calculated_ )
    {
        nlohmann::json serialized_fixture_data;
        serialized_fixture_data[kThroughput] = fixture_data.second.throughput;
        result[fixture_data.first.Serialize()] = serialized_fixture_data;
    }
    return result;
}
