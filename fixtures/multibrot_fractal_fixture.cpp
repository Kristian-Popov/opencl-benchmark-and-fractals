#include "fixtures/multibrot_fractal_fixture.h"
#include "utils.h"
#include <boost/format.hpp>

namespace
{
    // TODO due to this organization, all these variables classes collide
    // with other files, isolate them somehow
    static const char* programCode = R"(
/*
Requires a definition:
- floating point type REAL_T (float, or, if device supports, half and double),
*/

//#define MAX_ITERATION_NUMBER USHRT_MAX
#define MAX_ITERATION_NUMBER 1000

ushort4 CalcPointOnMultibrotSet( REAL_T real, REAL_T img, REAL_T power )
{
    ushort iterationNumber = 0;
    REAL_T escapePointSqr = 2*2;
    REAL_T zreal = 0;
    REAL_T zimg = 0;
    REAL_T zlenSqr = zreal*zreal + zimg*zimg;
    while (zlenSqr < escapePointSqr && iterationNumber < MAX_ITERATION_NUMBER)
    {
        // Formula for z^p is a multi-value function, by we use a basic plane only
        REAL_T multiplier = powr( (REAL_T)(zlenSqr), (REAL_T)(0.5*power) );
        // TODO danger zone here - at first iteration atan2 gets zero as both arguments
        REAL_T phi = atan2( zimg, zreal );
        zreal = multiplier * cos( power * phi ) + real;
        zimg = multiplier * sin( power * phi ) + img;

        zlenSqr = zreal*zreal + zimg*zimg;
        ++iterationNumber;
        //printf("%d\n", iterationNumber);
    }
    // For now we don't use color, so just return an iteration number in all components
    iterationNumber = ( USHRT_MAX / MAX_ITERATION_NUMBER ) * iterationNumber;
    iterationNumber = USHRT_MAX - iterationNumber;
    return (ushort4)( iterationNumber, iterationNumber, iterationNumber, USHRT_MAX );
}

/*
    rmin, rmax contain minimum and maximum values for real part of a values in an estimated range,
    imin, imax contain minimum and maximum values for imaginary part.

    Output is a pointer to buffer that will contain color data. Data are located in the following order:
    red, green, blue, alpha. Alpha value is always maximum possible value
*/
__kernel void MultibrotSetKernel(REAL_T rmin, REAL_T rmax, REAL_T imin, REAL_T imax, REAL_T power,
    __global ushort4* output)
{
    REAL_T rstep = ( rmax - rmin ) / get_global_size(0);
    REAL_T istep = ( imax - imin ) / get_global_size(1);
    
    size_t rowWidth = get_global_size(1);
    size_t resultIndex = get_global_id(0) * rowWidth + get_global_id(1);
    /*
    printf("%u;%u\n", get_global_id(0), get_global_id(1));
    printf("AAA %u\n", rowWidth);
    printf("BBB %u\n", get_global_size(1));
    */
    //printf("CCC %u\n", resultIndex);

    REAL_T real = rmin + get_global_id(0) * rstep;
    REAL_T img = imin + get_global_id(1) * istep;
    output[resultIndex] = CalcPointOnMultibrotSet( real, img, power );
}
)";

    template<typename T>
    struct Constants
    {
        static const char* openclTypeName;
        static const char* requiredExtension;
        static const char* descriptionTypeName;
        //static const int maxIterations;
    };

    const char* baseCompilerOptions = "-w -Werror";

    const char* Constants<cl_float>::openclTypeName = "float";
    const char* Constants<cl_float>::requiredExtension = ""; // No extensions needed for single precision arithmetic
    const char* Constants<cl_float>::descriptionTypeName = "float";
    //const int Constants<cl_float>::maxIterations = 20; //TODO update value

    const char* Constants<cl_double>::openclTypeName = "double";
    const char* Constants<cl_double>::requiredExtension = "cl_khr_fp64";
    const char* Constants<cl_double>::descriptionTypeName = "double";
    //const int KochCurveFixtureConstants<cl_double>::maxIterations = 20;

    // TODO support for half precision could be added
}

template<typename T>
inline MultibrotFractalFixture<T>::MultibrotFractalFixture( size_t width, size_t height,
    T realMin, T realMax, T imgMin, T imgMax, T power, const std::string & descriptionSuffix )
    : width_( width )
    , height_( height )
    , realMin_( realMin )
    , realMax_( realMax )
    , imgMin_( imgMin )
    , imgMax_( imgMax )
    , power_( power )
    , descriptionSuffix_( descriptionSuffix )
{
    EXCEPTION_ASSERT( width > 0 );
    EXCEPTION_ASSERT( height > 0 );
}

template<typename T>
inline void MultibrotFractalFixture<T>::InitializeForContext( boost::compute::context & context )
{
    std::string typeName = Constants<T>::openclTypeName;
    std::string compilerOptions = ( boost::format(
        "%1% -DREAL_T=%2%" ) %
        baseCompilerOptions %
        typeName
        ).str();

    kernels_.insert( {context.get(),
        Utils::BuildKernel( "MultibrotSetKernel", context, programCode, compilerOptions,
            GetRequiredExtensions() )} );
}

template<typename T>
inline std::vector<std::string> MultibrotFractalFixture<T>::GetRequiredExtensions()
{
    std::string requiredExtension = Constants<T>::requiredExtension;
    std::vector<std::string> result;
    if( !requiredExtension.empty() )
    {
        result.push_back( requiredExtension );
    }
    return result;
}

template<typename T>
inline std::unordered_map<OperationStep, Fixture::Duration> MultibrotFractalFixture<T>::Execute( boost::compute::context & context )
{
    EXCEPTION_ASSERT( context.get_devices().size() == 1 );
    outputData_.clear();
    // create command queue with profiling enabled
    boost::compute::command_queue queue(
        context, context.get_device(), boost::compute::command_queue::enable_profiling
    );

    std::unordered_multimap<OperationStep, boost::compute::event> events;

    boost::compute::vector<cl_ushort4> output_device_vector( GetElementsCountInternal(), context );

    boost::compute::kernel& kernel = kernels_.at( context.get() );

    kernel.set_arg( 0, realMin_ );
    kernel.set_arg( 1, realMax_ );
    kernel.set_arg( 2, imgMin_ );
    kernel.set_arg( 3, imgMax_ );
    kernel.set_arg( 4, power_ );
    kernel.set_arg( 5, output_device_vector.get_buffer() );

    boost::compute::extents<2> workgroupSize = {width_, height_};
    events.insert( {OperationStep::Calculate,
        queue.enqueue_nd_range_kernel( kernel, 2, nullptr, workgroupSize.data(), nullptr )
    } );

    boost::compute::event event;
    void* outputDataPtr = queue.enqueue_map_buffer_async(
        output_device_vector.get_buffer(), CL_MAP_WRITE, 0,
        output_device_vector.size() * sizeof( cl_ushort4 ),
        event );
    events.insert( {OperationStep::MapOutputData, event} );
    event.wait();

    const cl_ushort4* outputDataPtrCasted = reinterpret_cast<const cl_ushort4*>( outputDataPtr );
    outputData_.reserve( GetElementsCountInternal() );
    std::copy_n( outputDataPtrCasted, GetElementsCountInternal(), std::back_inserter( outputData_ ) );
    /*
    // Code for 2D output data map
    const cl_ushort4* outputDataPtrCastedBase = reinterpret_cast<const cl_ushort4*>( outputDataPtr );
    for (size_t r = 0; r < height_; ++r)
    {
        const cl_ushort4* outputDataPtrCasted = outputDataPtrCastedBase + r*width_;
        std::vector<cl_ushort4> row( width_ );
        std::copy_n( outputDataPtrCasted, width_, row.begin() );
        outputData_.push_back( row );
    }
    */

    boost::compute::event lastEvent = queue.enqueue_unmap_buffer( output_device_vector.get_buffer(), outputDataPtr );
    events.insert( {OperationStep::UnmapOutputData, lastEvent} );
    lastEvent.wait();

    return Utils::CalculateTotalStepDurations<Fixture::Duration>( events );
}

template<typename T>
inline std::string MultibrotFractalFixture<T>::Description()
{
    std::string fixtureBaseName = power_ == 2 ? "Mandelbrot set" : "Multibrot set";
    // TODO add power for Multibrot variant
    std::string result = ( boost::format( "%1%, %2%x%3%, %4%" ) %
        fixtureBaseName %
        width_ %
        height_ %
        Constants<T>::descriptionTypeName
        ).str();
    if( !descriptionSuffix_.empty() )
    {
        result += ", " + descriptionSuffix_;
    }
    return result;
}