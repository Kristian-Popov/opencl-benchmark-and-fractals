#pragma once

#include <iterator>
#include "iterators/data_source.h"

/*
    Iterator that generates sequential values of a given type.
    It is a constant (input only) forward iterators that return values of template type T.
    First returned value is equal to constructor parameter "startValue",
    incrementing iterator increases returned value by "step".
    Every value can be retrieved using dereferencing multiple times.
    This iterator is always referenceable.
*/
template <typename T>
class SequentialValuesIterator final :
    public std::iterator<std::forward_iterator_tag, T>,
    public DataSource<T>
{
public:
    SequentialValuesIterator( T startValue, T step )
        : value_(startValue)
        , step_(step)
    {}

    SequentialValuesIterator( const SequentialValuesIterator<T>& ) = default;
    SequentialValuesIterator<T>& operator=(const SequentialValuesIterator<T>& ) = default;

    bool operator==( const SequentialValuesIterator<T>& rhs )
    {
        return ( step_ == rhs.step_ ) && ( value_ == rhs.value_ );
    }
    bool operator!=( const SequentialValuesIterator<T>& rhs )
    {
        return !( *this == rhs );
    }

    T operator*()
    {
        return Get();
    }

    const T operator*() const
    {
        return value_;
    }

    SequentialValuesIterator<T>& operator++()
    {
        Increment();
        return *this;
    }

    const SequentialValuesIterator<T> operator++(int)
    {
        SequentialValuesIterator<T> temp = *this;
        ++(*this);
        return temp;
    }

    T Get() override
    {
        return value_;
    }

    void Increment() override
    {
        value_ += step_;
    }
private:
    T value_;
    T step_;
};