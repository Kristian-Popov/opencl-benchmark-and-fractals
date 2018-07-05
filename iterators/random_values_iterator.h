#pragma once

#include <iterator>
#include "iterators/data_source.h"

#include <boost/random/taus88.hpp>

/*
    Iterator that generates random values of a given type using a distribution given as a
    constructor parameter.
    It is an input iterators that return values of template type T.
    Every value can be retrieved only once, dereferencing again will produce another random value.
    This iterator is always referenceable.
*/
template <typename T, typename D>
class RandomValuesIterator final :
    public std::iterator<std::input_iterator_tag, T>,
    public DataSource<T>
{
public:
    RandomValuesIterator( const D& distribution )
        : distribution_( distribution )
        , generator_( std::random_device()() ) // TODO Not sure if this initialization is good
    {}

    RandomValuesIterator( const RandomValuesIterator<T, D>& ) = default;
    RandomValuesIterator<T, D>& operator=(const RandomValuesIterator<T, D>& ) = default;

    bool operator==( const RandomValuesIterator<T, D>& rhs )
    {
        return distribution_ == rhs.distribution_ && generator_ == rhs.generator_;
    }
    bool operator!=( const RandomValuesIterator<T, D>& rhs )
    {
        return !( *this == rhs );
    }

    T operator*()
    {
        return Get();
    }

    RandomValuesIterator<T, D>& operator++()
    {
        return *this;
    }

    const RandomValuesIterator<T, D> operator++(int)
    {
        RandomValuesIterator<T, D> temp = *this;
        ++(*this);
        return temp;
    }

    T Get() override
    {
        return distribution_( generator_ );
    }

    void Increment() override
    {
        // Do nothing here, next value will be different anyways
    }
private:
    /* Choosing a random number generator is a bit tricky here.
    A popular choice is Mersenne twister (e.g. std::mt19937), but according to
    http://www.boost.org/doc/libs/1_65_1/doc/html/boost_random/reference.html#boost_random.reference.generators
    it has a pretty big memory footprint ( 625*sizeof(uint32_t) ).
    Of course different implementations doesn't have to behave in this way, but I believe
    it is pretty common since it is an algorithm property.
    So instead boost::random::taus88 is used. It is just as fast, but smaller and has
    an acceptable period
    */
    boost::random::taus88 generator_;
    D distribution_;
};