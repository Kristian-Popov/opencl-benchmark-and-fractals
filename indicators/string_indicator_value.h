#include "indicators/indicator_value_interface.h"

class StringIndicatorValue: public IndicatorValueInterface
{
public:
    StringIndicatorValue( const std::string& val )
        : val_( val )
    {}

    std::string AsString() override
    {
        return val_;
    }

    void SetMultiplier( double multiplier ) override
    {
        // Do nothing, multiplier is not applicable to string indicator value
    }

    boost::optional<NumericType> GetNumericValue() override
    {
        return boost::optional<NumericType>();
    }
private:
    std::string val_;
};