#include "indicators/indicator_interface.h"

#include "utils/duration.h"
#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class ElementProcessingTimeIndicator: public IndicatorInterface
{
public:
    struct FixtureCalculatedData
    {
        std::unordered_map<OperationStep, Duration> step_durations;
        boost::optional<std::string> failure_reason;
        Duration total_duration;
    };

    ElementProcessingTimeIndicator()
    {}

    explicit ElementProcessingTimeIndicator( const BenchmarkResultForFixtureFamily& benchmark )
    {
        Calculate( benchmark );
    }

    nlohmann::json SerializeValue() override;

    std::unordered_map<FixtureId, FixtureCalculatedData> GetCalculatedData() const noexcept
    {
        return calculated_;
    }
private:
    void Calculate( const BenchmarkResultForFixtureFamily& benchmark );

    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
