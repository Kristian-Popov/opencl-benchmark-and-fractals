#pragma once

/*
This header file includes half precision floating point library by Christian Rau
and sets all required macros.
DO NOT include that library directly since you will get inconsistent rounding
options
*/

#include <limits>  // For std::round_to_nearest

#define HALF_ROUND_STYLE std::round_to_nearest
#define HALF_ROUND_TIES_TO_EVEN 1
#include "half/half.hpp"
