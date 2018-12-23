#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "operation_step.h"
#include "fixtures/fixture.h"
#include "fixtures/fixture_id.h"

struct FixtureFamily
{
    std::string name;
    std::vector<OperationStep> operation_steps;
    std::unordered_map<FixtureId, std::shared_ptr<Fixture>> fixtures;
};
