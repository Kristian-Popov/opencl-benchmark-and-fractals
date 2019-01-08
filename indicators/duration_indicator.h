#include "indicators/indicator_interface.h"

#include "utils/duration.h"
#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class DurationIndicator: public IndicatorInterface
{
public:
    struct FixtureCalculatedData
    {
        std::unordered_map<OperationStep, Duration> step_durations;
        boost::optional<std::string> failure_reason; // TODO is it needed in every indicator?
        Duration total_duration;
    };

    DurationIndicator() noexcept
    {}

    explicit DurationIndicator( const BenchmarkResultForFixtureFamily& benchmark )
    {
        Calculate( benchmark );
    }

    std::unordered_map<FixtureId, FixtureCalculatedData> GetCalculatedData() const noexcept
    {
        return calculated_;
    }

    nlohmann::json SerializeValue() override;
private:
    void Calculate( const BenchmarkResultForFixtureFamily& benchmark );

    std::string fixture_family_name_; // TODO is it needed?
    std::vector<OperationStep> operation_steps_;
    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
