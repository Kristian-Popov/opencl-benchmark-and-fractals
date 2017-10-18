#pragma once

#include "damped_wave_fixture.h"

#include <memory>
#include <random>
#include <cmath>

#include "data_verification_failed_exception.h"
#include "utils.h"

#include "half/half.hpp"

namespace
{
    const char* kernelCode = R"(
// Requires definition of REAL_T macro, it should be one of floating point types (either float or double)
// Also if support for a particular extension is needed, macro REQUIRED_EXTENSION should be defined
#ifdef REQUIRED_EXTENSION
#   pragma OPENCL EXTENSION REQUIRED_EXTENSION : enable
#endif
// TODO add unit test to check if size of this structure is the same as in host size.
typedef struct Parameters
{
    REAL_T amplitude;
    REAL_T dampingRatio;
    REAL_T angularFrequency;
    REAL_T phase;
    REAL_T shift;
} Parameters;

REAL_T DampedWave2DImplementation(REAL_T x, __global Parameters* params, int paramsCount)
{
    REAL_T result = 0;
    // TODO copy parameters to faster memory?
    for (int i = 0; i < paramsCount; ++i)
    {
        Parameters paramSet = params[i];
        REAL_T t = fabs(x - paramSet.shift);
        result += paramSet.amplitude * exp(-paramSet.dampingRatio * t) * 
            cos(paramSet.angularFrequency * t + paramSet.phase);
    }
    return result;
}

__kernel void DampedWave2D(__global REAL_T* input,
        __global Parameters* params, int paramsCount,
        __global REAL_T* output)
{
    size_t id = get_global_id(0);
    output[id] = DampedWave2DImplementation(input[id], params, paramsCount);
}
)";

    const char* baseCompilerOptions = "-w -Werror";

    template<typename T>
    struct Constants
    {
        static const char* openCLTemplateTypeName;
        static const char* descriptionTypeName;
        static const T maxAcceptableRelError;
        static const T maxAcceptableAbsError;
        static const char* requiredExtension;
    };

    const char* Constants<cl_float>::openCLTemplateTypeName = "float";
    const char* Constants<cl_float>::descriptionTypeName = "float";
    const cl_float Constants<cl_float>::maxAcceptableRelError = 0.015f; // TODO errors is quite large. Investigate why
    const cl_float Constants<cl_float>::maxAcceptableAbsError = 0.003f;
    const char* Constants<cl_float>::requiredExtension = ""; // No extensions needed for single precision arithmetic

    const char* Constants<cl_double>::openCLTemplateTypeName = "double";
    const char* Constants<cl_double>::descriptionTypeName = "double";
    // TODO calculate appropriate error, for now float values are used
    const cl_double Constants<cl_double>::maxAcceptableRelError = 0.015;
    const cl_double Constants<cl_double>::maxAcceptableAbsError = 0.003;
    const char* Constants<cl_double>::requiredExtension = "cl_khr_fp64";

    using namespace half_float::literal;
    const char* Constants<half_float::half>::openCLTemplateTypeName = "half";
    const char* Constants<half_float::half>::descriptionTypeName = "half";
    // TODO calculate appropriate error, for now float values are used
    const half_float::half Constants<half_float::half>::maxAcceptableRelError = 0.015_h;
    const half_float::half Constants<half_float::half>::maxAcceptableAbsError = 0.003_h;
    const char* Constants<half_float::half>::requiredExtension = "cl_khr_fp16";
}

template<typename T, typename I>
DampedWaveFixture<T, I>::DampedWaveFixture( const std::vector<DampedWaveFixtureParameters<T>>& params,
    const I& inputDataIter, size_t dataSize,
    const std::string& descriptionSuffix = std::string() )
    : params_( params )
    , inputDataIter_( inputDataIter )
    , dataSize_( dataSize )
    , descriptionSuffix_( descriptionSuffix )
{
    // TODO this doesn't work on half precision values, so disabled for now
    //static_assert( std::is_floating_point<T>::value, "DampedWaveFixture fixture works only for floating point types" );
    static_assert( FLT_EVAL_METHOD == 0, "Promotion of floating point values is enabled, please disable it using compiler options" );
    //TODO set rounding mode on host processor using std::fesetround
}

template<typename T, typename I>
void DampedWaveFixture<T, I>::Initialize()
{
    GenerateData();
}

template<typename T, typename I>
void DampedWaveFixture<T, I>::InitializeForContext( boost::compute::context& context )
{
    std::string compilerOptions = baseCompilerOptions + std::string( " -DREAL_T=" ) + 
        Constants<T>::openCLTemplateTypeName;

    std::string requiredExtension = Constants<T>::requiredExtension;
    if ( !requiredExtension.empty() )
    {
        compilerOptions += " -DREQUIRED_EXTENSION=" + requiredExtension;
    }
    kernels_.insert( {context.get(),
        Utils::BuildKernel( "DampedWave2D", context, kernelCode, compilerOptions )} );
}

template<typename T, typename I>
std::vector<OperationStep> DampedWaveFixture<T, I>::GetSteps()
{
    return{
        OperationStep::CopyInputDataToDevice, //TODO try replace copying with mapping
        OperationStep::Calculate,
        OperationStep::CopyOutputDataFromDevice
    };
}

template<typename T, typename I>
std::vector<std::string> DampedWaveFixture<T, I>::GetRequiredExtensions()
{
    std::string requiredExtension = Constants<T>::requiredExtension;
    std::vector<std::string> result;
    if ( !requiredExtension.empty() )
    {
        result.push_back( requiredExtension );
    }
    return result;
}

template<typename T, typename I>
std::unordered_map<OperationStep, Fixture::Duration> DampedWaveFixture<T, I>::Execute( 
    boost::compute::context& context )
{
    EXCEPTION_ASSERT( context.get_devices().size() == 1 );
    // create command queue with profiling enabled
    boost::compute::command_queue queue(
        context, context.get_device(), boost::compute::command_queue::enable_profiling
    );

    std::unordered_multimap<OperationStep, boost::compute::event> events;

    // create a vector on the device
    boost::compute::vector<T> input_device_vector( inputData_.size(), context );

    // copy data from the host to the device
    events.insert( {OperationStep::CopyInputDataToDevice,
        boost::compute::copy_async( inputData_.begin(), inputData_.end(), input_device_vector.begin(), queue ).get_event()
    } );

    // TODO measure time for copying parameters
    boost::compute::vector<Parameters> input_params_vector( params_.size(), context );
    events.insert( {OperationStep::CopyInputDataToDevice,
        boost::compute::copy_async( params_.begin(), params_.end(), input_params_vector.begin(), queue ).get_event()
    } );

    boost::compute::kernel& kernel = kernels_.at( context.get() );
    boost::compute::vector<T> output_device_vector( inputData_.size(), context );

    kernel.set_arg( 0, input_device_vector );
    kernel.set_arg( 1, input_params_vector );
    kernel.set_arg( 2, static_cast<cl_int>( params_.size() ) );
    kernel.set_arg( 3, output_device_vector );

    unsigned computeUnitsCount = context.get_device().compute_units();
    size_t localWorkGroupSize = 0;
    events.insert( {OperationStep::Calculate,
        queue.enqueue_1d_range_kernel( kernel, 0, inputData_.size(), localWorkGroupSize )
    } );

    outputData_.resize( inputData_.size() );
    boost::compute::event lastEvent = boost::compute::copy_async( output_device_vector.begin(), output_device_vector.end(), outputData_.begin(), queue ).get_event();
    events.insert( {OperationStep::CopyOutputDataFromDevice, lastEvent} );

    lastEvent.wait();

    return Utils::CalculateTotalStepDurations<Fixture::Duration>( events );
}

template<typename T, typename I>
std::string DampedWaveFixture<T, I>::Description()
{
    std::string result = ( boost::format( "Damped wave, %1%, %2% values, %3% parameters" ) %
        Constants<T>::descriptionTypeName %
        Utils::FormatQuantityString( dataSize_ ) %
        Utils::FormatQuantityString( params_.size() ) ).str();
    if( !descriptionSuffix_.empty() )
    {
        result += ", " + descriptionSuffix_;
    }
    return result;
}

template<typename T, typename I>
void DampedWaveFixture<T, I>::VerifyResults()
{
    EXCEPTION_ASSERT( !outputData_.empty() );
    EXCEPTION_ASSERT( outputData_.size() == inputData_.size() );
    EXCEPTION_ASSERT( outputData_.size() == expectedOutputData_.size() );
    EXCEPTION_ASSERT( outputData_.size() == dataSize_ );

    for( size_t i = 0; i < outputData_.size(); ++i )
    {
        bool areClose = Utils::AreFloatValuesClose(
            outputData_.at( i ), expectedOutputData_.at( i ),
            Constants<T>::maxAcceptableAbsError, Constants<T>::maxAcceptableRelError );
        if( !areClose )
        {
            throw DataVerificationFailedException( ( boost::format(
                "Result verification has failed for fixture \"%1%\". "
                "Found error for input value %2% (value index %3%)." ) %
                Description() % inputData_.at( i ) % i ).str() );
        }
    }
}

template<typename T, typename I>
std::vector<std::vector<T>> DampedWaveFixture<T, I>::GetResults()
{
    // Verify that output data are not empty to check if fixture was executed
    EXCEPTION_ASSERT( !outputData_.empty() );
    EXCEPTION_ASSERT( outputData_.size() == inputData_.size() );
    EXCEPTION_ASSERT( outputData_.size() == expectedOutputData_.size() );
    EXCEPTION_ASSERT( outputData_.size() == dataSize_ );
    std::vector<std::vector<T>> result;
    for( size_t index = 0; index < outputData_.size(); ++index )
    {
        result.push_back( {inputData_.at( index ),
            expectedOutputData_.at( index ),
            outputData_.at( index )} );
    }
    return result;
}

template<typename T, typename I>
void DampedWaveFixture<T, I>::GenerateData()
{
    inputData_.resize( dataSize_ );
    std::copy_n( inputDataIter_, dataSize_, inputData_.begin() );

    expectedOutputData_.reserve( dataSize_ );

    using namespace std::placeholders;
    std::transform( inputData_.begin(), inputData_.end(), std::back_inserter( expectedOutputData_ ),
        std::bind( &DampedWaveFixture<T, I>::CalcExpectedValue, this, _1 ) );
}
/*
template<typename T, typename I>
T DampedWaveFixture<T, I>::CalcExpectedValue( T x ) const
{
    return std::accumulate<decltype( params_.cbegin() ), T>( params_.cbegin(), params_.cend(), 0,
        [x]( T accum, const Parameters& param )
    {
        T v = std::abs( x - param.shift );
        T currentWave = param.amplitude * std::exp( -param.dampingRatio * v ) * cos( param.angularFrequency * v + param.phase );
        return accum + currentWave;
    } );
}
*/

template<typename T, typename I>
T DampedWaveFixture<T, I>::CalcExpectedValue( T x ) const
{
    return std::accumulate<decltype( params_.cbegin() ), T>( params_.cbegin(), params_.cend(), static_cast<T>(0),
        [x]( T accum, const Parameters& param )
    {
        T v = abs( x - param.shift );
        T currentWave = param.amplitude * exp( -param.dampingRatio * v ) * cos( param.angularFrequency * v + param.phase );
        return accum + currentWave;
    } );
}