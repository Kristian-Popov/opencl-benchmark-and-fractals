#include "utils/duration.h"
#include "reporters/benchmark_results.h"

#include <unordered_map>
#include <boost/optional.hpp>

class DurationIndicator
{
public:
    explicit DurationIndicator( const FixtureBenchmark& benchmark )
    {
        Calculate( benchmark );
    }

    void SerializeValue( nlohmann::json& tree );

    bool IsEmpty() const;
private:
    struct FixtureCalculatedData
    {
        std::unordered_map<OperationStep, Duration> step_durations;
        std::unordered_map<OperationStep, Duration> step_min_durations;
        std::unordered_map<OperationStep, Duration> step_max_durations;
        std::size_t iteration_count;
        // Is not serialized, is just a temporary solution to check if duration is empty
        // TODO check in some better way?
        Duration total_duration;
    };

    void Calculate( const FixtureBenchmark& benchmark );

    FixtureCalculatedData calculated_;
};
