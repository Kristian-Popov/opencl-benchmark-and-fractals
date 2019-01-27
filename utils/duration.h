#pragma once

#include <chrono>
#include <string>

#include "nlohmann/json.hpp"

using nlohmann::json;

namespace boost
{
    namespace compute
    {
        class event;
    }
}

class Duration
{
public:
    constexpr Duration() noexcept
        : duration_( InternalType::zero() )
    {}

    template<typename D>
    explicit Duration( D duration )
        : duration_( std::chrono::duration_cast<InternalType>( duration ) )
    {
        if( duration < D::zero() )
        {
            throw std::invalid_argument( "Attempt to construct a negative duration." );
        }
    }

    explicit Duration( const boost::compute::event& event );

    Duration& operator=( const Duration& d )
    {
        this->duration_ = d.duration_;
        return *this;
    }

    template<typename D>
    Duration& operator=( D duration )
    {
        if( duration < D::zero() )
        {
            throw std::invalid_argument( "Attempt to construct a negative duration." );
        }
        this->duration_ = std::chrono::duration_cast<InternalType>( duration );
        return *this;
    }

    Duration& operator+=( const Duration& d )
    {
        duration_ += d.duration_;
        return *this;
    }
#if 0
    // Disabled for now.
    // Duration cannot be below zero, what to do when requested e.g. Duration(1s)-Duration(2s)?
    Duration& operator-=( const Duration& d )
    {
        duration_ -= d.duration_;
        return *this;
    }
#endif
    template<typename T>
    Duration& operator*=( T m )
    {
        static_assert( std::is_arithmetic<T>::value, "Duration operator*= second argument is not an arithmetic value." );
        duration_ *= m;
        return *this;
    }

    template<typename T>
    Duration& operator/=( T m )
    {
        static_assert( std::is_arithmetic<T>::value, "Duration operator/= second argument is not an arithmetic value." );
        duration_ /= m;
        return *this;
    }

    double AsSeconds() const;

    bool operator==( const Duration& rhs ) const noexcept
    {
        return duration_ == rhs.duration_;
    }

    bool operator!=( const Duration& rhs ) const noexcept
    {
        return !( *this == rhs );
    }

    std::chrono::duration<double, std::nano> duration() const noexcept
    {
        return duration_;
    }

    friend Duration operator+( Duration lhs, Duration rhs )
    {
        lhs += rhs;
        return lhs;
    }
#if 0
    friend Duration operator-( Duration lhs, Duration rhs )
    {
        lhs -= rhs;
        return lhs;
    }
#endif
    bool operator<( const Duration& rhs ) const noexcept
    {
        return duration_ < rhs.duration_;
    }

    bool operator<=( const Duration& rhs ) const noexcept
    {
        return duration_ <= rhs.duration_;
    }

    bool operator>( const Duration& rhs ) const noexcept
    {
        return duration_ > rhs.duration_;
    }
    bool operator>=( const Duration& rhs ) const noexcept
    {
        return duration_ >= rhs.duration_;
    }

    static const Duration Min() noexcept
    {
        // By some reason InternalType::min() duration is negative,
        // no idea how duration can be below zero
        // Using zero instead
        return Duration( InternalType::zero() );
    }

    static const Duration Max() noexcept
    {
        return Duration( InternalType::max() );
    }

    template<typename T>
    friend Duration operator*( Duration lhs, T rhs )
    {
        lhs *= rhs;
        return lhs;
    }

    template<typename T>
    friend Duration operator/( Duration lhs, T rhs )
    {
        lhs /= rhs;
        return lhs;
    }

    friend double operator/( Duration lhs, Duration rhs )
    {
        return lhs.duration_ / rhs.duration_;
    }

private:
    /*
        Internally duration is stored as double-precision floating point count of nanoseconds
        OpenCL uses cl_ulong (same as uint64_t - 64-bit usigned integer number).
        It is perfect to specify duration of the whole operation, but it is not
        good when divided by very large number (e.g. 9mcs=900 000 ns divided by 1M elements
        is just 0 ns per element, when using floating point we get approx. 0.9 ns per element,
        which may bring more useful information).
        Storing it in nanoseconds improves accurancy greatly, max duration that can be fit
        without losing accurancy is 52 days, so more than enough.
    */
    typedef std::chrono::duration<double, std::nano> InternalType;

    InternalType duration_;
};

void to_json( json& j, const Duration& p );
void from_json( const json& j, Duration& p );