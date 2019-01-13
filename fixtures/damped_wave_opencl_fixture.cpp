#pragma once

#include "csv_document.h"
#include "fixtures/damped_wave_opencl_fixture.h"
#include "half_precision_fp.h"
#include "utils.h"

#include "boost/format.hpp"

namespace
{
    const char* dampedWaveProgramCode = R"(
// Requires definition of REAL_T macro, it should be one of floating point types (either float or double)
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
        // TODO why do we need to take absolute value below?
        REAL_T t = x - paramSet.shift;
        result += paramSet.amplitude * exp(-paramSet.dampingRatio * max( t, (REAL_T)0 )) *
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

    const char* baseCompilerOptions = "-Werror";

    template<typename T>
    struct DampedWaveOpenClFixtureConstants
    {
        static const char* openCLTemplateTypeName;
        static const char* requiredExtension;
    };

    const char* DampedWaveOpenClFixtureConstants<cl_float>::openCLTemplateTypeName = "float";
    const char* DampedWaveOpenClFixtureConstants<cl_float>::requiredExtension = ""; // No extensions needed for single precision arithmetic

    const char* DampedWaveOpenClFixtureConstants<cl_double>::openCLTemplateTypeName = "double";
    const char* DampedWaveOpenClFixtureConstants<cl_double>::requiredExtension = "cl_khr_fp64";

    const char* DampedWaveOpenClFixtureConstants<half_float::half>::openCLTemplateTypeName = "half";
    const char* DampedWaveOpenClFixtureConstants<half_float::half>::requiredExtension = "cl_khr_fp16";
}

template<typename T>
DampedWaveOpenClFixture<T>::DampedWaveOpenClFixture(
    const std::shared_ptr<OpenClDevice>& device,
    const std::vector<DampedWaveFixtureParameters<T>>& params,
    const std::shared_ptr<DataSource<T>>& input_data_source,
    size_t dataSize,
    const std::string& fixture_name
)
    : device_( device )
    , params_( params )
    , input_data_source_( input_data_source )
    , dataSize_( dataSize )
    , fixture_name_( fixture_name )
{
}

template<typename T>
void DampedWaveOpenClFixture<T>::Initialize()
{
    GenerateData();
    std::string compilerOptions = baseCompilerOptions + std::string( " -DREAL_T=" ) +
        DampedWaveOpenClFixtureConstants<T>::openCLTemplateTypeName;
    kernel_ = Utils::BuildKernel( "DampedWave2D", device_->GetContext(), dampedWaveProgramCode, compilerOptions,
        GetRequiredExtensions() );
}

template<typename T>
std::vector<std::string> DampedWaveOpenClFixture<T>::GetRequiredExtensions()
{
    std::string requiredExtension = DampedWaveOpenClFixtureConstants<T>::requiredExtension;
    std::vector<std::string> result;
    if ( !requiredExtension.empty() )
    {
        result.push_back( requiredExtension );
    }
    return result;
}

template<typename T>
std::unordered_multimap<OperationStep, Duration> DampedWaveOpenClFixture<T>::Execute()
{
    boost::compute::context& context = device_->GetContext();
    boost::compute::command_queue& queue = device_->GetQueue();
    std::unordered_multimap<OperationStep, boost::compute::event> events;

    // create a vector on the device
    boost::compute::vector<T> input_device_vector( inputData_.size(), context );

    // copy data from the host to the device
    events.insert( {OperationStep::CopyInputDataToDevice,
        boost::compute::copy_async( inputData_.begin(), inputData_.end(), input_device_vector.begin(), queue ).get_event()
    } );

    boost::compute::vector<Parameters> input_params_vector( params_.size(), context );
    events.insert( {OperationStep::CopyInputDataToDevice,
        boost::compute::copy_async( params_.begin(), params_.end(), input_params_vector.begin(), queue ).get_event()
    } );

    boost::compute::vector<T> output_device_vector( inputData_.size(), context );

    kernel_.set_arg( 0, input_device_vector );
    kernel_.set_arg( 1, input_params_vector );
    EXCEPTION_ASSERT( params_.size() <= std::numeric_limits<cl_int>::max() );
    kernel_.set_arg( 2, static_cast<cl_int>( params_.size() ) );
    kernel_.set_arg( 3, output_device_vector );

    unsigned computeUnitsCount = context.get_device().compute_units();
    size_t localWorkGroupSize = 0;
    events.insert( {OperationStep::Calculate,
        queue.enqueue_1d_range_kernel( kernel_, 0, inputData_.size(), localWorkGroupSize )
    } );

    outputData_.resize( inputData_.size() );
    boost::compute::event lastEvent = boost::compute::copy_async( output_device_vector.begin(), output_device_vector.end(), outputData_.begin(), queue ).get_event();
    events.insert( {OperationStep::CopyOutputDataFromDevice, lastEvent} );

    lastEvent.wait();

    return Utils::GetOpenCLEventDurations( events );
}

template<typename T>
void DampedWaveOpenClFixture<T>::GenerateData()
{
    inputData_.resize( dataSize_ );
    std::copy_n( DataSourceAdaptor<T>{ input_data_source_ }, dataSize_, inputData_.begin() );
}

template<typename T>
std::vector<std::vector<T>> DampedWaveOpenClFixture<T>::GetResults()
{
    // Verify that output data are not empty to check if fixture was executed
    EXCEPTION_ASSERT( !outputData_.empty() );
    EXCEPTION_ASSERT( outputData_.size() == inputData_.size() );
    EXCEPTION_ASSERT( outputData_.size() == dataSize_ );
    std::vector<std::vector<T>> result;
    for( size_t index = 0; index < outputData_.size(); ++index )
    {
        result.push_back( { inputData_.at( index ), outputData_.at( index )} );
    }
    return result;
}

template<typename T>
void DampedWaveOpenClFixture<T>::StoreResults()
{
    const std::string file_name = ( boost::format( "%1%, %2%.csv" ) %
        fixture_name_ %
        device_->Name() ).str();
    CSVDocument csvDocument( file_name );
    csvDocument.AddValues( GetResults() );
    csvDocument.BuildAndWriteToDisk();
}