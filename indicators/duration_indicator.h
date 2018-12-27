#include "indicators/indicator_interface.h"

#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class DurationIndicator: public IndicatorInterface
{
public:
    struct FixtureCalculatedData
    {
        std::unordered_map<OperationStep, DurationType> step_durations;
        boost::optional<std::string> failure_reason; // TODO is it needed in every indicator?
        DurationType total_duration = DurationType::zero();
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

    boost::property_tree::ptree SerializeValue() override;
private:
    void Calculate( const BenchmarkResultForFixtureFamily& benchmark );

    std::string fixture_family_name_; // TODO is it needed?
    std::vector<OperationStep> operation_steps_;
    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
