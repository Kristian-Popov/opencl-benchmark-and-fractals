#include "indicators/indicator_interface.h"

#include "utils/duration.h"
#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class ElementProcessingTimeIndicator: public IndicatorInterface
{
public:
    ElementProcessingTimeIndicator()
    {}

    explicit ElementProcessingTimeIndicator( const BenchmarkResultForFixtureFamily& benchmark )
    {
        Calculate( benchmark );
    }

    boost::property_tree::ptree SerializeValue() override;
private:
    void Calculate( const BenchmarkResultForFixtureFamily& benchmark );

    struct FixtureCalculatedData
    {
        std::unordered_map<OperationStep, Duration> step_durations;
        boost::optional<std::string> failure_reason;
        Duration total_duration;
    };

    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
