#include "indicators/indicator_interface.h"

#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class ThroughputIndicator: public IndicatorInterface
{
public:
    ThroughputIndicator()
    {}

    explicit ThroughputIndicator( const BenchmarkResultForFixtureFamily& benchmark )
    {
        Calculate( benchmark );
    }

    boost::property_tree::ptree SerializeValue() override;
private:
    void Calculate( const BenchmarkResultForFixtureFamily& benchmark );

    struct FixtureCalculatedData
    {
        boost::optional<std::string> failure_reason;
        int32_t throughput = 0;
    };

    std::unordered_map<FixtureId, FixtureCalculatedData> calculated_;
};
