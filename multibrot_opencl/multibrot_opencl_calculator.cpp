#include "multibrot_opencl_calculator.h"

#include <boost/format.hpp>

#include <unordered_map>

namespace
{
    static const char* kMainProgram = R"(
/*
Requires a definition:
- REAL_T floating point type (acceptable types are float and, if device supports, half and double),
- RESULT_T - type of result pixel component (either uchar or ushort)
- RESULT_MAX - max value accepted by RESULT_T (either CHAR_MAX or USHRT_MAX)
- POWER_FUNC - name of a function that executes power of Z and adds C
  Function must take the following parameters:
  - Z = (zreal, zimg) - input and output number
  - zlen_sqr - square of absolute value of Z (square)
  - power
  - C = (real, img) - the second addend
  In general this function should do the following mathematical operation:
  (zreal, zimg) = (zreal, zimg) ^ power + (real, img)
  Different functions are used for optimisation purposes
*/

// Preprocessor magic based on https://stackoverflow.com/a/1489985
#define PASTER_2(x,y) x ## y
#define PASTER_3(x,y,z) x ## y ## z
#define EVALUATOR_2(x,y)  PASTER_2(x,y)
#define EVALUATOR_3(x,y,z)  PASTER_3(x,y,z)
#define CONVERT EVALUATOR_3(convert_, RESULT_T, _sat)
#define REAL_T4 EVALUATOR_2(REAL_T, 4)

// Universal complex number power function, supports all real powers but the slowest
void UniversalPowerOfComplex(
    __private REAL_T* restrict zreal,
    __private REAL_T* restrict zimg,
    const REAL_T zlen_sqr,
    const REAL_T power,
    const REAL_T real,
    const REAL_T img
)
{
    // Formula for z^p is a multi-value function, by we use a basic plane only
    REAL_T multiplier = powr( (REAL_T)(zlen_sqr), (REAL_T)(0.5*power) );
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
    const REAL_T zlen_sqr,
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
    const REAL_T zlen_sqr,
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
    const REAL_T zlen_sqr,
    const REAL_T power,
    const REAL_T real,
    const REAL_T img
)
{
    REAL_T zreal_new = pown(*zreal, 3) - 3 * (*zreal) * (*zimg) * (*zimg) + real;
    *zimg = 3 * (*zreal) * (*zreal) * (*zimg) - pown(*zimg, 3) + img;
    *zreal = zreal_new;
}

/*
    Calculate number of iterations that point after which point
    escaped the set.
    Returns raw number of iterations.
*/
REAL_T CalcPointOnMultibrotSet( REAL_T real, REAL_T img, REAL_T power, ushort max_iter_number )
{
    REAL_T iter_number = 0;
    REAL_T zreal = 0;
    REAL_T zimg = 0;
    REAL_T zlen_sqr = 0;
    while ( zlen_sqr < 2*2 && iter_number < max_iter_number )
    {
        POWER_FUNC( &zreal, &zimg, zlen_sqr, power, real, img );

        zlen_sqr = zreal*zreal + zimg*zimg;
        iter_number += 1;
    }
    return iter_number;
}

#ifdef COLOR_ENABLED
/*
    Scales iteration number and picks a color.
    May be used only when RESULT_T is uchar4 or ushort4.
    Useful for building color images.
*/
RESULT_T ProcessIterationNumber( REAL_T iter_number, ushort max_iter_number )
{
    // Calculate a color in HSV
    // This algorithm is based on https://www.codingame.com/playgrounds/2358/how-to-plot-the-mandelbrot-set/adding-some-colors
    REAL_T value = iter_number < max_iter_number ? RESULT_MAX : 0;
    REAL_T hue = ( RESULT_MAX / max_iter_number ) * iter_number;
    // Saturation is always max.
    // Convert to RGB
    REAL_T hue_section = hue / ( RESULT_MAX / 6 ); // hue_section is in range [0; 6]
    uchar i1 = convert_uchar_sat_rtz( hue_section / 2 );
    uchar i2 = convert_uchar_sat_rtz( remainder( hue_section, 2 ) );
    uchar ic = ( i1 + i2 ) % 3;
    uchar ix = ( i1 - i2 + 1 ) % 3;

    REAL_T rgb[3] = { 0 };
    const REAL_T chroma = value; // * saturation
    REAL_T second_color = chroma * ( 1 - fabs( remainder( hue_section, 2 ) - 1 ) );
    rgb[ic] = chroma;
    rgb[ix] = second_color;

    REAL_T4 result = (REAL_T4)( vload3( 0, rgb ), RESULT_MAX );
    return CONVERT( result );
}
#else
/*
    Scales iteration number so it uses all available values from 0 to max supported by a given
    number type.
    May be used only when RESULT_T is uchar or ushort.
    Useful for building grayscale images.
*/
RESULT_T ProcessIterationNumber( REAL_T iter_number, ushort max_iter_number )
{
    iter_number = ( RESULT_MAX / max_iter_number ) * iter_number;
    iter_number = RESULT_MAX - iter_number;
    return CONVERT( iter_number );
}
#endif

/*
    Build an image of Mandelbrot or Multibrot set.

    (rmin, imin) - complex value that corresponds to the left top corner of the image,
    (rmax, imax) - right bottom corner.

    Output is a pointer to buffer that will contain pixel data. Data format is defined by RESULT_T.

    Must be called with two-dimensional work assignment, where dimensions are used as image
    size (first dimension is width, second dimension is height).
    Pixels are stored in row-major order.
*/
__kernel void MultibrotSetKernel(
    REAL_T rmin, REAL_T imin,
    REAL_T rmax, REAL_T imax,
    REAL_T power,
    ushort max_iter_number,
    __global RESULT_T* restrict output
)
{
    REAL_T rstep = ( rmax - rmin ) / get_global_size(0);
    REAL_T istep = ( imax - imin ) / get_global_size(1);

    size_t row_width = get_global_size(0);
    size_t result_index = get_global_id(1) * row_width + get_global_id(0);

    REAL_T real = rmin + get_global_id(0) * rstep;
    REAL_T img = imin + get_global_id(1) * istep;
    REAL_T multibrot_val = CalcPointOnMultibrotSet( real, img, power, max_iter_number );
    RESULT_T result = ProcessIterationNumber( multibrot_val, max_iter_number );
    output[result_index] = result;
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
        static const bool color_enabled;
    };

    // Grayscale 8 bit
    const char* ResultTypeConstants<cl_uchar>::result_type_name = "uchar";
    const char* ResultTypeConstants<cl_uchar>::result_max_val_macro = "UCHAR_MAX";
    const int ResultTypeConstants<cl_uchar>::result_max_val = CL_UCHAR_MAX;
    const bool ResultTypeConstants<cl_uchar>::color_enabled = false;

    // Grayscale 16 bit
    const char* ResultTypeConstants<cl_ushort>::result_type_name = "ushort";
    const char* ResultTypeConstants<cl_ushort>::result_max_val_macro = "USHRT_MAX";
    const int ResultTypeConstants<cl_ushort>::result_max_val = CL_USHRT_MAX;
    const bool ResultTypeConstants<cl_ushort>::color_enabled = false;

    // RGB 8 bit
    const char* ResultTypeConstants<cl_uchar4>::result_type_name = "uchar4";
    const char* ResultTypeConstants<cl_uchar4>::result_max_val_macro = "UCHAR_MAX";
    const int ResultTypeConstants<cl_uchar4>::result_max_val = CL_UCHAR_MAX;
    const bool ResultTypeConstants<cl_uchar4>::color_enabled = true;

    // RGB 16 bit
    const char* ResultTypeConstants<cl_ushort4>::result_type_name = "ushort4";
    const char* ResultTypeConstants<cl_ushort4>::result_max_val_macro = "USHRT_MAX";
    const int ResultTypeConstants<cl_ushort4>::result_max_val = CL_USHRT_MAX;
    const bool ResultTypeConstants<cl_ushort4>::color_enabled = true;
}

template<typename T, typename P>
MultibrotOpenClCalculator<T, P>::MultibrotOpenClCalculator(
    const boost::compute::device& device,
    const boost::compute::context& context,
    size_t max_width_pix, size_t max_height_pix
)
    : device_( device )
    , context_( context )
    , queue_( context, device, boost::compute::command_queue::enable_profiling )
    , max_width_pix_( max_width_pix )
    , max_height_pix_( max_height_pix )
    , output_device_vector_( max_width_pix * max_height_pix, context )
{
    BuildKernels();
}

template<typename T, typename P>
std::string MultibrotOpenClCalculator<T, P>::PrepareCompilerOptions( const std::string& power_func )
{
    return ( boost::format(
        "-Werror -DREAL_T=%1% -DRESULT_T=%2% -DRESULT_MAX=%3% "
        "-DPOWER_FUNC=%4% %5% " ) %
        TempValueConstants<T>::opencl_type_name %
        ResultTypeConstants<P>::result_type_name %
        ResultTypeConstants<P>::result_max_val_macro %
        power_func %
        ( ResultTypeConstants<P>::color_enabled ? "-DCOLOR_ENABLED" : "" )
    ).str();
}

template<typename T, typename P>
void MultibrotOpenClCalculator<T, P>::BuildKernels()
{
    // Collecting required extensions
    std::string required_extension = TempValueConstants<T>::required_extension;
    std::vector<std::string> extensions;
    if( !required_extension.empty() )
    {
        extensions.push_back( required_extension );
    }

    static const std::unordered_map<double /* power */, std::string> kFixedPowerFunctions = {
        { 1.0, "Power1OfComplex" },
        { 2.0, "SquareOfComplex" },
        { 3.0, "CubeOfComplex" },
    };

    for( const auto& d: kFixedPowerFunctions )
    {
        specialized_kernels_.emplace( d.first,
            Utils::BuildKernel( "MultibrotSetKernel", context_, kMainProgram,
                PrepareCompilerOptions( d.second ), extensions )
        );
    }

    universal_kernel_ = Utils::BuildKernel( "MultibrotSetKernel", context_, kMainProgram,
        PrepareCompilerOptions( "UniversalPowerOfComplex" ), extensions );
}

template<typename T, typename P>
void MultibrotOpenClCalculator<T, P>::ExecutePrecalculateChecks(
    size_t width_pix, size_t height_pix, int max_iterations )
{
    EXCEPTION_ASSERT( width_pix <= max_width_pix_ );
    EXCEPTION_ASSERT( height_pix <= max_height_pix_ );
    // Verify that given max iterations is valid for given pixel bit depth
    EXCEPTION_ASSERT( max_iterations <= ResultTypeConstants<P>::result_max_val );
}