#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "utils/duration.h"
#include "fixtures/fixture_family.h"
#include "fixtures/fixture_id.h"
#include "operation_step.h"

#include "boost/optional.hpp"

struct BenchmarkResultForFixture
{
    std::vector<std::unordered_map<OperationStep, Duration>> durations;
    boost::optional<std::string> failure_reason;
};

struct BenchmarkResultForFixtureFamily
{
    std::unordered_map<FixtureId, BenchmarkResultForFixture> benchmark;
    std::shared_ptr<FixtureFamily> fixture_family;
};
