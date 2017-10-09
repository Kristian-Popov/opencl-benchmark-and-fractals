#pragma once

#include <memory>
#include <random>
#include <cmath>

#include "utils.h"
#include "fixture.h"

#include <boost/format.hpp>

namespace
{
    const char* kernelCode = R"(
// Requires definition of REAL_T macro, it should be one of floating point types (either float or double)
// TODO can half precision be used here?
REAL_T DampedWave2DImplementation(REAL_T x, REAL_T amplitude, REAL_T dampingRatio, REAL_T angularFrequency, REAL_T phase)
{
    x = fabs(x);
    return amplitude * exp(-dampingRatio * x) * cos(angularFrequency * x + phase);
}

__kernel void DampedWave2D(__global REAL_T* input,
        REAL_T amplitude, REAL_T dampingRatio, REAL_T angularFrequency, REAL_T phase,
        __global REAL_T* output)
{
    size_t id = get_global_id(0);
    output[id] = DampedWave2DImplementation(input[id], amplitude, dampingRatio, angularFrequency, phase);
}
)";

    const char* baseCompilerOptions = "-w -Werror";
}

template<typename T>
class DampedWaveFixture : public Fixture
{
public:
    enum class DataPattern
    {
        Linear,
        Random
    };

    DampedWaveFixture( T amplitude, T dampingRatio, T angularFrequency, T phase, T min, T max, DataPattern dataPattern, T step = 0 )
        : amplitude_(amplitude)
        , dampingRatio_(dampingRatio)
        , angularFrequency_( angularFrequency )
        , phase_( angularFrequency )
        , min_(min)
        , max_(max)
        , step_(step)
        , dataPattern_( dataPattern )
    {
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

    std::unordered_map<OperationStep, ExecutionResult> Execute( boost::compute::context& context ) override
    {
        std::unordered_map<OperationStep, ExecutionResult> result;

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
        kernel.set_arg( 1, amplitude_ );
        kernel.set_arg( 2, dampingRatio_ );
        kernel.set_arg( 3, angularFrequency_ );
        kernel.set_arg( 4, phase_ );
        kernel.set_arg( 5, output_device_vector );

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
            result.insert( std::make_pair( v.first, ExecutionResult( v.first, v.second.duration<Duration>() ) ) );
        }

        return result;
    }

    std::string Description() override
    {
        return std::string( "Damped wave, " ) + descriptionTypeName;
    }

    void VerifyResults()
    {
        EXCEPTION_ASSERT( outputData_.size() == expectedOutputData_.size() );
        std::vector<T> relError;
        std::transform( outputData_.cbegin(), outputData_.cend(), expectedOutputData_.cbegin(), std::back_inserter( relError ),
            []( T got, T expected )
        {
            return std::abs( ( got - expected ) / got );
        } );
        auto maxRelErrorIter = std::max_element( relError.cbegin(), relError.cend() );
        T maxRelError = *maxRelErrorIter;
        auto index = std::distance( relError.cbegin(), maxRelErrorIter );
        EXCEPTION_ASSERT( index >= 0 && index < outputData_.size() );
        T inputValueThatGivesMaxError = *( outputData_.cbegin() + index );

        if( maxRelError > maxAcceptableRelError )
        {
            throw std::runtime_error( ( boost::format( "Result verification has failed for fixture \"%1%\". "
                "Maximum relative error is %2% for input value %3% "
                "(maximum acceptable error is %4%)" ) %
                Description() % maxRelError % inputValueThatGivesMaxError % maxAcceptableRelError ).str() );
        }
    }

    virtual ~DampedWaveFixture()
    {
    }
private:
    std::vector<T> inputData_;
    std::vector<T> expectedOutputData_;
    std::vector<T> outputData_;
    DataPattern dataPattern_;
    T amplitude_ = 0, dampingRatio_ = 0, angularFrequency_ = 0, phase_ = 0, min_ = 0, max_ = 0, step_ = 0;
    static const char* openCLTemplateTypeName;
    static const char* descriptionTypeName;
    static const T maxAcceptableRelError;

    void GenerateData()
    {
        EXCEPTION_ASSERT( dataPattern_ == DataPattern::Linear ); // Random is not supported yet
        size_t dataSize = static_cast<size_t>(std::ceil((max_ - min_) / step_));
        inputData_.resize( dataSize );

        T val = min_;
        std::generate( inputData_.begin(), inputData_.end(), 
            [&val, this] ()
            {
                T temp = val;
                val += step_;
                return temp;
            } );

        // Verify that all input values are in range [min, max]
        EXCEPTION_ASSERT( std::all_of( inputData_.begin(), inputData_.end(), [this]( T x ) { return x >= min_ && x <= max_; } ) );

        expectedOutputData_.reserve( dataSize );
        std::transform( inputData_.begin(), inputData_.end(), std::back_inserter( expectedOutputData_ ),
            [this]( T x )
            {
                x = std::abs(x); 
                return amplitude_ * std::exp(-dampingRatio_ * x) * cos( angularFrequency_ * x + phase_ );
            } );
#if 0
        // Specify the engine and distribution.
        std::mt19937 mersenne_engine;
        std::uniform_int_distribution<int> dist( 0, 20 );

        auto gen = std::bind( dist, mersenne_engine );
        std::generate( begin( inputData_ ), end( inputData_ ), gen );

        // Verify that all input values are in range [0, 20]
        EXCEPTION_ASSERT( std::all_of( inputData_.begin(), inputData_.end(), []( int i ) { return i >= 0 && i <= 20; } ) );

        expectedOutputData_.reserve( dataSize_ );
        std::transform( inputData_.begin(), inputData_.end(), std::back_inserter( expectedOutputData_ ),
            [this]( int i )
        {
            return correctFactorialValues_.at( i );
        } );
#endif
    }
};

const char* DampedWaveFixture<cl_float>::openCLTemplateTypeName = "float";
const char* DampedWaveFixture<cl_float>::descriptionTypeName = "float";
const cl_float DampedWaveFixture<cl_float>::maxAcceptableRelError = 1e-3f; // TODO max error is quite large (0.00095). Investigate why

/*
Not supported yet
const char* DampledWaveFixture<cl_double>::openCLTemplateTypeName = "double";
const char* DampledWaveFixture<cl_double>::descriptionTypeName = "double";
*/