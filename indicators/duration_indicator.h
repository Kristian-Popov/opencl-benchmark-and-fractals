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
        std::unordered_map<OperationStep, Duration> step_min_durations;
        std::unordered_map<OperationStep, Duration> step_max_durations;
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

    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
