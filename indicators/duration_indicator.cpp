#include "indicators/duration_indicator.h"

#include "fixtures/fixture_family.h"
#include "indicators/string_indicator_value.h"
#include "indicators/numeric_indicator_value.h"
#include <utils.h>

namespace
{
	const char* const kFailureReason = "Failure reason";
	const char* const kTotalDuration = "Total duration";
}

void DurationIndicator::Calculate( const BenchmarkResultForFixtureFamily& benchmark )
{
	operation_steps_ = benchmark.fixture_family->operation_steps;
	fixture_family_name_ = benchmark.fixture_family->name;

    for( auto& fixture_data: benchmark.benchmark )
    {
        const BenchmarkResultForFixture& fixture_results = fixture_data.second;
        FixtureCalculatedData d;
        if( fixture_results.failure_reason )
        {
			d.failure_reason = fixture_results.failure_reason.value();
		}
        else
        {
            // Calculate duration for every step and total one.
			DurationType total_duration = DurationType::zero();
            for( int stepIndex = 0; stepIndex < operation_steps_.size(); ++stepIndex )
            {
                OperationStep step = operation_steps_.at( stepIndex );

                for( auto& results: fixture_results.durations )
                {
                    auto range = results.equal_range( step );
                    DurationType val = std::accumulate( range.first, range.second, DurationType::zero(), []
                        ( DurationType lhs, const std::pair<OperationStep, DurationType>& rhs ) -> DurationType
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
            // Very large element counts may cause problems, double and std::size_t may have same size
            EXCEPTION_ASSERT( element_count <= 1e9 );
            const double element_count_double = static_cast<double>( element_count );
            total_duration /= element_count_double;
            for( auto& p: d.step_durations )
            {
                p.second /= element_count_double;
            }

            d.total_duration = total_duration;
        }
        calculated_.insert( std::make_pair( fixture_data.first, d ) );
    }
}

boost::property_tree::ptree DurationIndicator::SerializeValue()
{
	// TODO serialize operation_steps_ and fixture_family_name_?
    namespace pr_tree = boost::property_tree;
	boost::property_tree::ptree result;
	for( auto& fixture_data: calculated_ )
	{
		boost::property_tree::ptree serialized_fixture_data;
		if( fixture_data.second.failure_reason )
		{
			serialized_fixture_data.put<std::string>( kFailureReason, fixture_data.second.failure_reason.value() );
		}
		else
		{
			serialized_fixture_data.put<std::string>( kTotalDuration, Utils::SerializeNumber( fixture_data.second.total_duration.count() ) );
			for( OperationStep step: operation_steps_ )
			{
				serialized_fixture_data.put<std::string>( OperationStepDescriptionRepository::GetSerializeId( step ),
                    Utils::SerializeNumber( fixture_data.second.step_durations[step].count() ) );
			}
		}
        result.push_back( pr_tree::ptree::value_type( fixture_data.first.Serialize(), serialized_fixture_data ) );
	}
	return result;
}
