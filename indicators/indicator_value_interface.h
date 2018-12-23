#pragma once

#include <string>

#include "reporters/benchmark_results.h"

//#include "operation_step.h"

#include "boost/optional.hpp"

/*
    Describes a single indicator value (for table-based reporters it is a single cell in a table)
*/
class IndicatorValueInterface
{
public:
    virtual std::string AsString() = 0;
    virtual void SetMultiplier( double multiplier ) = 0;
    virtual boost::optional<NumericType> GetNumericValue() = 0;
    virtual ~IndicatorValueInterface() noexcept {}
};