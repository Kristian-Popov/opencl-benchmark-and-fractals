#pragma once

#include "boost/compute.hpp"

namespace KochCurveUtils
{
#pragma pack(push, 1)
    template<typename T4>
    struct Line
    {
        T4 coords;
        // First number (x) is an iteration number, second is line identifier in current iteration
        cl_int2 ids;

        Line()
            : coords( { 0, 0, 0, 0 } )
            , ids( { 0, 0 } )
        {}

        Line(T4 coords_, cl_int2 ids_)
            : coords(coords_)
            , ids(ids_)
        {}
    };
#pragma pack(pop)
}