#include "fixtures/multibrot/multibrot_opencl_calculator.h"

#include <boost/format.hpp>

#include <unordered_map>

namespace
{
    // TODO due to this organization, all these variables classes collide
    // with other files, isolate them somehow
    static const char* kMainProgram = R"(
/*
Requires a definition:
- REAL_T floating point type (acceptable types are float and, if device supports, half and double),
- RESULT_T - type of result pixel component (either uchar or ushort)
- RESULT_MAX - max value accepted by RESULT_T (either CHAR_MAX or USHRT_MAX)
- MAX_ITER_NUMBER - max iteration number (should be smaller than or equal to RESULT_MAX)
- POWER_FUNC - name of a function that executes power of Z and adds C
  Function must take the following parameters:
  - Z = (zreal, zimg) - input and output number
  - zlenSqr - square of absolute value of Z (square)
  - power
  - C = (real, img) - the second addend
  In general this function should do the following mathematical operation:
  (zreal, zimg) = (zreal, zimg) ^ power + (real, img)
  Different functions are used for optimisation purposes
*/

// Universal complex number power function, supports all real powers but the slowest
void UniversalPowerOfComplex(
    __private REAL_T* restrict zreal,
    __private REAL_T* restrict zimg,
    const REAL_T zlenSqr,
    const REAL_T power,
    const REAL_T real,
    const REAL_T img
)
{
    // Formula for z^p is a multi-value function, by we use a basic plane only
    REAL_T multiplier = powr( (REAL_T)(zlenSqr), (REAL_T)(0.5*power) );
    // TODO danger zone here - at first iteration atan2 gets zero as both arguments
    // Fix UB somehow
    REAL_T phi = atan2( *zimg, *zreal );
    *zreal = multiplier * cos( power * phi ) + real;
    *zimg = multiplier * sin( power * phi ) + img;
}

// Should be used when power is 1
void Power1OfComplex(
    __private REAL_T* restrict zreal,
    __private REAL_T* restrict zimg,
    const REAL_T zlenSqr,
    const REAL_T power,
    const REAL_T real,
    const REAL_T img
)
{
    *zreal += real;
    *zimg += img;
}

// Should be used when power is 2
void SquareOfComplex(
    __private REAL_T* restrict zreal,
    __private REAL_T* restrict zimg,
    const REAL_T zlenSqr,
    const REAL_T power,
    const REAL_T real,
    const REAL_T img
)
{
    REAL_T zreal_new = (*zreal) * (*zreal) - (*zimg) * (*zimg) + real;
    *zimg = 2 * (*zreal) * (*zimg) + img;
    *zreal = zreal_new;
}

// Should be used when power is 3
void CubeOfComplex(
    __private REAL_T* restrict zreal,
    __private REAL_T* restrict zimg,
    const REAL_T zlenSqr,
    const REAL_T power,
    const REAL_T real,
    const REAL_T img
)
{
    REAL_T zreal_new = pown(*zreal, 3) - 3 * (*zreal) * (*zimg) * (*zimg) + real;
    *zimg = 3 * (*zreal) * (*zreal) * (*zimg) - pown(*zimg, 3) + img;
    *zreal = zreal_new;
}

RESULT_T CalcPointOnMultibrotSet( REAL_T real, REAL_T img, REAL_T power )
{
    RESULT_T iterationNumber = 0;
    REAL_T escapePointSqr = 2*2;
    REAL_T zreal = 0;
    REAL_T zimg = 0;
    REAL_T zlenSqr = 0;
    while (zlenSqr < escapePointSqr && iterationNumber < MAX_ITER_NUMBER)
    {
        POWER_FUNC( &zreal, &zimg, zlenSqr, power, real, img );

        zlenSqr = zreal*zreal + zimg*zimg;
        ++iterationNumber;
    }
    iterationNumber = ( RESULT_MAX / MAX_ITER_NUMBER ) * iterationNumber;
    iterationNumber = RESULT_MAX - iterationNumber;
    return iterationNumber;
}

/*
    rmin, rmax contain minimum and maximum values for real part of a values in an estimated range,
    imin, imax contain minimum and maximum values for imaginary part.

    Output is a pointer to buffer that will contain pixel data. Data format is defined by RESULT_T.
*/
__kernel void MultibrotSetKernel(REAL_T rmin, REAL_T rmax, REAL_T imin, REAL_T imax, REAL_T power,
    __global RESULT_T* restrict output)
{
    REAL_T rstep = ( rmax - rmin ) / get_global_size(0);
    REAL_T istep = ( imax - imin ) / get_global_size(1);

    // TODO is it inverted by accident?
    size_t rowWidth = get_global_size(1);
    size_t resultIndex = get_global_id(1) * rowWidth + get_global_id(0);

    REAL_T real = rmin + get_global_id(0) * rstep;
    REAL_T img = imin + get_global_id(1) * istep;
    output[resultIndex] = CalcPointOnMultibrotSet( real, img, power );
}
)";

    template<typename T>
    struct TempValueConstants {
        static const char* opencl_type_name;
        static const char* required_extension;
    };

    const char* TempValueConstants<half_float::half>::opencl_type_name = "half";
    const char* TempValueConstants<half_float::half>::required_extension = "cl_khr_fp16";

    const char* TempValueConstants<float>::opencl_type_name = "float";
    const char* TempValueConstants<float>::required_extension = "";

    const char* TempValueConstants<double>::opencl_type_name = "double";
    const char* TempValueConstants<double>::required_extension = "cl_khr_fp64";

    template<typename P>
    struct ResultTypeConstants {
        static const char* result_type_name;
        static const char* result_max_val_macro;
        static const int result_max_val;
    };

    // Grayscale 8 bit
    const char* ResultTypeConstants<cl_uchar>::result_type_name = "uchar";
    const char* ResultTypeConstants<cl_uchar>::result_max_val_macro = "UCHAR_MAX";
    const int ResultTypeConstants<cl_uchar>::result_max_val = CL_UCHAR_MAX;

    // Grapyscale 16 bit
    const char* ResultTypeConstants<cl_ushort>::result_type_name = "ushort";
    const char* ResultTypeConstants<cl_ushort>::result_max_val_macro = "USHRT_MAX";
    const int ResultTypeConstants<cl_ushort>::result_max_val = CL_USHRT_MAX;
}

template<typename T, typename P>
MultibrotOpenClCalculator<T, P>::MultibrotOpenClCalculator(
    const boost::compute::device& device,
    const boost::compute::context& context,
    size_t max_width_pix, size_t max_height_pix,
    double power,
    int max_iterations
)
    : device_( device )
    , context_( context )
    , queue_( context, device, boost::compute::command_queue::enable_profiling )
    , max_width_pix_( max_width_pix )
    , max_height_pix_( max_height_pix )
    , power_( power )
    , max_iterations_( max_iterations )
    , output_device_vector_( max_width_pix * max_height_pix, context )
{
    // Verify that given max iterations cis valid for given pixel bit depth
    EXCEPTION_ASSERT( max_iterations_ <= ResultTypeConstants<P>::result_max_val );

    // Compiler options
    std::string compiler_options =
        ( boost::format(
            "-Werror -DREAL_T=%1% -DRESULT_T=%2% -DRESULT_MAX=%3% "
            "-DMAX_ITER_NUMBER=%4% -DPOWER_FUNC=%5%" ) %
        TempValueConstants<T>::opencl_type_name %
        ResultTypeConstants<P>::result_type_name %
        ResultTypeConstants<P>::result_max_val_macro %
        max_iterations_ %
        SelectPowerFunction()
        ).str();

    // Collecting required extensions
    std::string required_extension = TempValueConstants<T>::required_extension;
    std::vector<std::string> extensions;
    if( !required_extension.empty() )
    {
        extensions.push_back( required_extension );
    }

    kernel_ = Utils::BuildKernel( "MultibrotSetKernel", context_, kMainProgram, compiler_options,
        extensions );
}

template<typename T, typename P>
std::string MultibrotOpenClCalculator<T, P>::SelectPowerFunction()
{
    static const std::unordered_map<double /* power */, std::string> kFixedPowerFunctions = {
        { 1.0, "Power1OfComplex" },
        { 2.0, "SquareOfComplex" },
        { 3.0, "CubeOfComplex" },
    };

    std::string result{ "UniversalPowerOfComplex" }; // Default universal function
    auto iter = kFixedPowerFunctions.find( power_ );
    if( iter != kFixedPowerFunctions.cend() )
    {
        result = iter->second;
    }
    return result;
}