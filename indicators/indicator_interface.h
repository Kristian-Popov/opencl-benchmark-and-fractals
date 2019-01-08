#pragma once

#include "fixtures/fixture_id.h"

#include <map>
#include <memory>

#include "nlohmann/json.hpp"

class IndicatorInterface
{
public:
    // TODO can we replace that with to_json?
    virtual nlohmann::json SerializeValue() = 0;

    virtual ~IndicatorInterface() noexcept {}
};
