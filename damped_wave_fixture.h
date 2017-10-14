#pragma once

#include <memory>
#include <random>
#include <cmath>

#include "data_verification_failed_exception.h"
#include "utils.h"
#include "fixture_that_returns_data.h"

#include <boost/format.hpp>

namespace
{
    const char* kernelCode = R"(
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
}

#pragma pack(push, 1)
template<typename T>
struct DampedWaveFixtureParameters
{
    T amplitude = 0;
    T dampingRatio = 0;
    T angularFrequency = 0;
    T phase = 0;
    T shift = 0;

    DampedWaveFixtureParameters( T amplitude_, T dampingRatio_, T angularFrequency_, T phase_, T shift_ )
        : amplitude( amplitude_ )
        , dampingRatio( dampingRatio_ )
        , angularFrequency( angularFrequency_ )
        , phase( phase_ )
        , shift( shift_ )
    {
    }
};
#pragma pack(pop)

template<typename T, typename I>
class DampedWaveFixture : public FixtureThatReturnsData<T>
{
public:
    DampedWaveFixture( const std::vector<DampedWaveFixtureParameters<T>>& params,
        const I& inputDataIter, size_t dataSize,
        const std::string& descriptionSuffix = std::string() )
        : params_( params )
        , inputDataIter_( inputDataIter )
        , dataSize_( dataSize )
        , descriptionSuffix_( descriptionSuffix )
    {
        static_assert( std::is_floating_point<T>::value, "DampedWaveFixture fixture works only for floating point types" );
        static_assert( FLT_EVAL_METHOD == 0, "Promotion of floating point values is enabled, please disable it using compiler options" );
    }

    virtual void Initialize() override
    {
        GenerateData();
    }

    virtual std::vector<OperationStep> GetSteps() override
    {
        return {
            OperationStep::CopyInputDataToDevice, //TODO try replace copying with mapping
            OperationStep::Calculate,
            OperationStep::CopyOutputDataFromDevice
        };
    }

    std::unordered_map<OperationStep, Duration> Execute( boost::compute::context& context ) override
    {
        std::unordered_map<OperationStep, Duration> result;

        EXCEPTION_ASSERT( context.get_devices().size() == 1 );
        // create command queue with profiling enabled
        boost::compute::command_queue queue(
            context, context.get_device(), boost::compute::command_queue::enable_profiling
        );

        std::unordered_map<OperationStep, boost::compute::event> events;

        // create a vector on the device
        boost::compute::vector<T> input_device_vector( inputData_.size(), context );

        // copy data from the host to the device
        events.insert( {OperationStep::CopyInputDataToDevice,
            boost::compute::copy_async( inputData_.begin(), inputData_.end(), input_device_vector.begin(), queue ).get_event()
        } );

        // TODO measure time for copying parameters
        boost::compute::vector<Parameters> input_params_vector( params_.size(), context );
        boost::compute::copy( params_.begin(), params_.end(), input_params_vector.begin(), queue );

        std::string compilerOptions = baseCompilerOptions + std::string(" -DREAL_T=") + openCLTemplateTypeName;
        boost::compute::program program;
        try
        {
            program = boost::compute::program::build_with_source( kernelCode, context, compilerOptions );
        }
        catch (boost::compute::opencl_error& error)
        {
            std::string source, buildOptions, buildLog;
            try // Retrieving these values can cause OpenCL errors, so catch them to write at least something about an error
            {
                source = program.source();
                buildOptions = program.get_build_info<std::string>( CL_PROGRAM_BUILD_OPTIONS, context.get_device() );
                buildLog = program.build_log();
            }
            catch( boost::compute::opencl_error& )
            {
            }

            if (error.error_code() == CL_BUILD_PROGRAM_FAILURE )
            {
                //TODO something weird happens with std::cerr here, using cout for now
                std::cerr << "Kernel for fixture \"" << Description() << "\" could not be built. Kernel source: " << std::endl <<
                    source << std::endl <<
                    "Build options: " << buildOptions << std::endl <<
                    "Build log: " << buildLog << std::endl;
            }
            throw;
        }
        boost::compute::kernel kernel( program, "DampedWave2D" );
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

        for( const std::pair<OperationStep, boost::compute::event>& v : events )
        {
            result.insert( std::make_pair( v.first, v.second.duration<Duration>() ) );
        }

        return result;
    }

    std::string Description() override
    {
        std::string result = ( boost::format( "Damped wave, %1%, %2% values, %3% parameters" ) %
            descriptionTypeName % 
            Utils::FormatQuantityString( dataSize_ ) % 
            Utils::FormatQuantityString( params_.size() ) ).str();
        if( !descriptionSuffix_.empty() )
        {
            result += ", " + descriptionSuffix_;
        }
        return result;
    }

    void VerifyResults()
    {
        EXCEPTION_ASSERT( !outputData_.empty() );
        EXCEPTION_ASSERT( outputData_.size() == inputData_.size() );
        EXCEPTION_ASSERT( outputData_.size() == expectedOutputData_.size() );
        EXCEPTION_ASSERT( outputData_.size() == dataSize_ );

        for (size_t i = 0; i < outputData_.size(); ++i )
        {
            bool areClose = Utils::AreFloatValuesClose( 
                outputData_.at(i), expectedOutputData_.at(i),
                maxAcceptableAbsError, maxAcceptableRelError );
            if (!areClose)
            {
                throw DataVerificationFailedException( ( boost::format( 
                    "Result verification has failed for fixture \"%1%\". "
                    "Found error for input value %2% (value index %3%).") %
                    Description() % inputData_.at(i) % i ).str() );
            }
        }
    }

    /*
        Return results in the following form:
        {
            { xmin, y_expected_1, y_1 },
            { x2, y_expected_2, y_2 },
            ...
            { xmax, y_expected_n, y_n },
        }
        First column contains input value, second one - expected value (as calculated on a host CPU),
        third one - value calculated by OpenCL device
    */
    virtual std::vector<std::vector<T>> GetResults() override
    {
        // Verify that output data are not empty to check if fixture was executed
        EXCEPTION_ASSERT( !outputData_.empty() );
        EXCEPTION_ASSERT( outputData_.size() == inputData_.size() );
        EXCEPTION_ASSERT( outputData_.size() == expectedOutputData_.size() );
        EXCEPTION_ASSERT( outputData_.size() == dataSize_ );
        std::vector<std::vector<T>> result;
        for (size_t index = 0; index < outputData_.size(); ++index )
        {
            result.push_back( { inputData_.at(index), 
                expectedOutputData_.at(index), 
                outputData_.at(index) } );
        }
        return result;
    }

    virtual boost::optional<size_t> GetElementsCount() override
    {
        return dataSize_;
    }

    virtual ~DampedWaveFixture()
    {
    }
private:
    typedef DampedWaveFixtureParameters<T> Parameters;

    std::vector<T> inputData_;
    std::vector<T> expectedOutputData_;
    std::vector<T> outputData_;
    std::vector<Parameters> params_;
    I inputDataIter_;
    size_t dataSize_;
    std::string descriptionSuffix_;
    static const char* openCLTemplateTypeName;
    static const char* descriptionTypeName;
    static const T maxAcceptableRelError;
    static const T maxAcceptableAbsError;

    void GenerateData()
    {
        inputData_.resize( dataSize_ );
        std::copy_n( inputDataIter_, dataSize_, inputData_.begin() ); 

        expectedOutputData_.reserve( dataSize_ );

        using namespace std::placeholders;
        std::transform( inputData_.begin(), inputData_.end(), std::back_inserter( expectedOutputData_ ), 
            std::bind( &DampedWaveFixture<T, I>::CalcExpectedValue, this, _1 ) );
    }

    T CalcExpectedValue( T x ) const
    {
        return std::accumulate<decltype( params_.cbegin() ), T>( params_.cbegin(), params_.cend(), 0,
            [x] (T accum, const Parameters& param)
            {
                T v = std::abs( x - param.shift );
                T currentWave = param.amplitude * std::exp( -param.dampingRatio * v ) * cos( param.angularFrequency * v + param.phase );
                return accum + currentWave;
            } );
    }
};

template<typename T = cl_float, typename I>
const char* DampedWaveFixture<T, I>::openCLTemplateTypeName = "float";
template<typename T = cl_float, typename I>
const char* DampedWaveFixture<T, I>::descriptionTypeName = "float";
template<typename T = cl_float, typename I>
const cl_float DampedWaveFixture<T, I>::maxAcceptableRelError = 0.015f; // TODO errors is quite large. Investigate why
template<typename T = cl_float, typename I>
const cl_float DampedWaveFixture<T, I>::maxAcceptableAbsError = 0.003f;

/*
Not supported yet
const char* DampledWaveFixture<cl_double>::openCLTemplateTypeName = "double";
const char* DampledWaveFixture<cl_double>::descriptionTypeName = "double";
*/