#pragma once

//TODO include this file in one place with correct configuration of rounding just before
// an include
#include "half/half.hpp"

#include <random>

class HalfPrecisionNormalDistribution
{
public:
    typedef half_float::half result_type;

    explicit HalfPrecisionNormalDistribution( 
        result_type mean = static_cast<result_type>( 0.0 ), 
        result_type sigma = static_cast<result_type>( 1.0 ) )
        : dist_( mean, sigma )
    {}

    void reset()
    {
        dist_.reset();
    }

    template<typename G>
    result_type operator()(G& engine)
    {
        return static_cast<result_type>(dist_(engine));
    }

    result_type min()
    {
        return static_cast<result_type>( dist_.min() );
    }

    result_type max()
    {
        return static_cast<result_type>( dist_.max() );
    }
private:
    std::normal_distribution<float> dist_;
};