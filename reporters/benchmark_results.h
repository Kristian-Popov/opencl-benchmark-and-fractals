#pragma once

#include <chrono>
#include <vector>
#include <unordered_map>
#include <boost/optional.hpp>

// TODO should include a fixture family, but this causes a circular dependency.

#include "fixtures/fixture_id.h"
#include "operation_step.h"

#include "boost/optional.hpp"

struct FixtureFamily;

typedef double NumericType; // TODO remove that to fix header dependencies
typedef std::chrono::duration<NumericType> DurationType;

struct BenchmarkResultForFixture
{
    std::vector<std::unordered_multimap<OperationStep, DurationType>> durations;
    boost::optional<std::string> failure_reason;
};

struct BenchmarkResultForFixtureFamily
{
    std::unordered_map<FixtureId, BenchmarkResultForFixture> benchmark;
    std::shared_ptr<FixtureFamily> fixture_family;
};
