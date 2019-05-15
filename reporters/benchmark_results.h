#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "boost/optional.hpp"
#include "fixtures/fixture_family.h"
#include "fixtures/fixture_id.h"
#include "operation_step.h"
#include "utils/duration.h"

struct FixtureBenchmark {
    std::vector<std::unordered_map<OperationStep, Duration>> durations;
    boost::optional<std::string> failure_reason;
};

struct FixtureFamilyBenchmark {
    std::unordered_map<FixtureId, FixtureBenchmark> benchmark;
    std::shared_ptr<FixtureFamily> fixture_family;  // TODO should this be weak_ptr?
};
