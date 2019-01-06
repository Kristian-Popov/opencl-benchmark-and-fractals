#pragma once

#include <chrono>
#include <string>

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
    Duration()
        : duration_( InternalType::zero() )
    {}

    template<typename D>
    explicit Duration( D duration )
        : duration_( std::chrono::duration_cast<InternalType>( duration ) )
    {}

    explicit Duration( const boost::compute::event& event );

    Duration& operator=( const Duration& d )
    {
        this->duration_ = d.duration_;
        return *this;
    }

    template<typename D>
    Duration& operator=( D duration )
    {
        this->duration_ = std::chrono::duration_cast<InternalType>( duration );
        return *this;
    }

    Duration& operator+=( const Duration& d )
    {
        duration_ += d.duration_;
        return *this;
    }

    Duration& operator-=( const Duration& d )
    {
        duration_ -= d.duration_;
        return *this;
    }

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

    std::string Serialize() const;

    double AsSeconds() const;

    friend Duration operator+( Duration lhs, Duration rhs )
    {
        lhs += rhs;
        return lhs;
    }

    friend Duration operator-( Duration lhs, Duration rhs )
    {
        lhs -= rhs;
        return lhs;
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
        is just 0 ns per element, when using floating point we get approx. 0.9 ns per element).
        Storing it in nanoseconds improves accurancy greatly, max duration that can be fit
        without losing accurancy is 52 days, so more than enough.
    */
    typedef std::chrono::duration<double, std::nano> InternalType;

    InternalType duration_;
};