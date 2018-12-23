#include "indicators/indicator_interface.h"

#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class DurationIndicator: public IndicatorInterface
{
public:
	DurationIndicator()
	{}

    explicit DurationIndicator( const BenchmarkResultForFixtureFamily& benchmark )
    {
		Calculate( benchmark );
	}

    boost::property_tree::ptree SerializeValue() override;
private:
	void Calculate( const BenchmarkResultForFixtureFamily& benchmark );

	struct FixtureCalculatedData
	{
		std::unordered_map<OperationStep, DurationType> step_durations;
		boost::optional<std::string> failure_reason;
		DurationType total_duration = DurationType::zero();
	};

    std::string fixture_family_name_;
    std::vector<OperationStep> operation_steps_;
    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
