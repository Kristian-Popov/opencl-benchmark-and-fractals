#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/optional.hpp"
#include "fixtures/fixture.h"
#include "fixtures/fixture_id.h"

struct FixtureFamily {
    std::string name;
    std::unordered_map<FixtureId, std::shared_ptr<Fixture>> fixtures;
    boost::optional<int32_t> element_count;
};
