#include "indicators/indicator_interface.h"

#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class ThroughputIndicator: public IndicatorInterface
{
public:
    struct FixtureCalculatedData
    {
        int32_t throughput = 0;
    };

    ThroughputIndicator()
    {}

    explicit ThroughputIndicator( const BenchmarkResultForFixtureFamily& benchmark )
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
