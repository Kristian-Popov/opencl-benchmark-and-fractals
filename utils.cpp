#include "utils.h"

#include "fixture.h"

#include <type_traits>

namespace Utils
{
    std::string ReadFile( const std::string& fileName )
    {
        std::ifstream stream( fileName );
        std::stringbuf buf;
        if( !stream.is_open() )
        {
            throw std::runtime_error( "File not found" );
        }
        stream >> &buf;
        stream.close();
        return buf.str();
    }

    std::string FormatQuantityString( int value )
    {
        std::string result;
        if (value % 1000000 == 0)
        {
            result = std::to_string(value / 1000000) + "M";
        }
        else if( value % 1000 == 0 )
        {
            result = std::to_string( value / 1000 ) + "K";
        }
        else
        {
            result = std::to_string( value);
        }
        return result;
    }

    // Based on this article
    // https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
    template<typename T>
    bool AreFloatValuesClose( T A, T B,
        T maxAbsDiff, T maxRelDiff ) // TODO not sure about of these differences - T or e.g. long double?
    {
        static_assert( std::is_floating_point<T>::value, "AreFloatValuesClose function works only for floating point types" );
        // Check if the numbers are really close -- needed
        // when comparing numbers near zero.
        long double diff = fabs( A - B );
        if( diff <= maxAbsDiff )
        {
            return true;
        }

        A = fabs( A );
        B = fabs( B );
        T largest = std::max( A, B );

        return diff <= largest * maxRelDiff;
    }

    long double ChooseConvenientUnit( long double value, 
        const std::vector<long double>& units )
    {
        EXCEPTION_ASSERT( !units.empty() );
        EXCEPTION_ASSERT( std::is_sorted( units.begin(), units.end() ) );
        // First the biggest unit that is smaller than input value
        auto resultIter = std::find_if( units.crbegin(), units.crend(),
            [value] (long double unit )
            {
                return unit < value;
            } );
        long double result = 1.0f;
        if ( resultIter == units.crend() )
        {
            result = *units.begin();
        }
        else
        {
            result = *resultIter;
        }
        return result;
    }

    long double ChooseConvenientUnit( const std::vector<long double>& values,
        const std::vector<long double>& units )
    {
        std::vector<long double> convUnits;
        convUnits.reserve( values.size() );

        std::transform( values.begin(), values.end(), std::back_inserter( convUnits ),
            [&units] (long double value)
            {
                return ChooseConvenientUnit(value, units);
            } );

        // Count how many times every unit occurs
        std::unordered_map<long double, int> counts;
        for( long double unit: convUnits )
        {
            ++counts[unit];
        }
        EXCEPTION_ASSERT( !counts.empty() );
        // TODO find a unit that occurs most frequently
		auto resultIter = std::max_element( counts.begin(), counts.end(), 
            CompareSecond<long double, int> );
        return resultIter->first;
    }
}
