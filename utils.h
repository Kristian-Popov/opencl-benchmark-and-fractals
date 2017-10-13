#pragma once

#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <memory>

#include "boost/compute/compute.hpp"

class Fixture;

namespace Utils
{
    std::string ReadFile( const std::string& fileName );

    template <typename T>
    std::string VectorToString( const std::vector<T>& v )
    {
        std::stringstream result;
        std::copy( v.begin(), v.end(), std::ostream_iterator<T>( result, ", " ) );
        return result.str();
    }

    template <typename T>
    void AppendVectorToVector( T& v1, const T& v2 )
    {
        v1.insert( v1.end(), v2.begin(), v2.end() );
    }

    template <typename D, typename T>
    D MeasureDuration( boost::compute::future<T>& future )
    {
        future.wait();
        return future.get_event().duration<D>();
    }

    template <typename D>
    D MeasureDuration( boost::compute::event& event )
    {
        event.wait();
        return event.duration<D>();
    }

    /*
        Formats text representation of large values:
        - for 1-1000, returned as is
        - for 1000-999000 when last three digits are zeroes - 1K, 999K
        - for 1000000-999000000 when last six digits are zeroes - 1M, 999M
    */
    std::string FormatQuantityString(int value);

    template<typename T>
    bool AreFloatValuesClose( T A, T B,
        T maxAbsDiff, T maxRelDiff ); // TODO not sure about of these differences - T or e.g. long double?

    template bool AreFloatValuesClose<float>( float A, float B, float maxAbsDiff, float maxRelDiff );
    template bool AreFloatValuesClose<double>( double A, double B, double maxAbsDiff, double maxRelDiff );
}

#define EXCEPTION_ASSERT(expr) { if(!(expr)) { throw std::logic_error("Assert \"" #expr "\" failed"); } }
