#pragma once

#include "indicators/indicator_value_interface.h"
#include "fixtures/fixture_id.h"

#include <map>
#include <memory>

#include <boost/property_tree/ptree.hpp>

class IndicatorInterface
{
public:
    struct CellInfo
    {
        std::shared_ptr<IndicatorValueInterface> indicator_;
        std::size_t columnSpan_ = 1;
    };

    virtual boost::property_tree::ptree SerializeValue() = 0;

    virtual ~IndicatorInterface() noexcept {}
};
