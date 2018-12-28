#pragma once

#include "fixtures/fixture_id.h"

#include <map>
#include <memory>

#include <boost/property_tree/ptree.hpp>

class IndicatorInterface
{
public:
    virtual boost::property_tree::ptree SerializeValue() = 0;

    virtual ~IndicatorInterface() noexcept {}
};
