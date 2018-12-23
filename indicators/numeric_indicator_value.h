#include "indicators/indicator_value_interface.h"

class NumericIndicatorValue: public IndicatorValueInterface
{
public:
    NumericIndicatorValue( double val )
        : val_( val )
    {}

    std::string AsString() override
    {
        return std::to_string( val_ * multiplier_ );
    }

    void SetMultiplier( double multiplier ) override
    {
        multiplier_ = multiplier;
    }

    boost::optional<NumericType> GetNumericValue() override
    {
        return val_ * multiplier_;
    }
private:
    double val_;
    double multiplier_ = 1.0;
};