#pragma once

#include <memory>
#include <random>

#include "utils.h"
#include "fixture.h"

#include <boost/format.hpp>

namespace
{
    const char* trivialFactorialKernelCode = R"(
ulong FactorialImplementation(int val)
{
    ulong result = 1;
    for(int i = 1; i <= val; i++)
    {
        result *= i;
    }
    return result;
}

__kernel void TrivialFactorial(__global int* input, __global ulong* output)
{
    size_t id = get_global_id(0);
    output[id] = FactorialImplementation(input[id]);
}
)"; 

    const std::string compilerOptions = "-w -Werror";
}

template<typename I>
class TrivialFactorialFixture: public Fixture
{
public:
    TrivialFactorialFixture( const I& inputDataIter, int dataSize )
        : inputDataIter_( inputDataIter )
        , dataSize_(dataSize)
	{
	}

    virtual void Initialize() override
    {
        GenerateData();
    }

    virtual std::vector<OperationStep> GetSteps() override
    {
        return {
            OperationStep::CopyInputDataToDevice,
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
        boost::compute::vector<int> input_device_vector( dataSize_, context );

        // copy data from the host to the device
        events.insert( { OperationStep::CopyInputDataToDevice, 
            boost::compute::copy_async( inputData_.begin(), inputData_.end(), input_device_vector.begin(), queue ).get_event() 
        } );

        boost::compute::program program = boost::compute::program::build_with_source( trivialFactorialKernelCode, context, compilerOptions );
        boost::compute::kernel kernel( program, "TrivialFactorial" );
        boost::compute::vector<unsigned long long> output_device_vector( dataSize_, context );
        kernel.set_arg( 0, input_device_vector );
        kernel.set_arg( 1, output_device_vector );

        unsigned computeUnitsCount = context.get_device().compute_units();
        size_t localWorkGroupSize = 0;
        events.insert( { OperationStep::Calculate,
            queue.enqueue_1d_range_kernel( kernel, 0, dataSize_, localWorkGroupSize )
        } );

        outputData_.resize( dataSize_ );
        boost::compute::event lastEvent = boost::compute::copy_async( output_device_vector.begin(), output_device_vector.end(), outputData_.begin(), queue ).get_event();
        events.insert( { OperationStep::CopyOutputDataFromDevice, lastEvent } );

        lastEvent.wait();

        for ( const std::pair<OperationStep, boost::compute::event>& v: events )
        {
            result.insert( std::make_pair( v.first, v.second.duration<Duration>() ) );
        }

        return result;
	}

    std::string Description() override
    {
        return std::string("Trivial factorial, ") + Utils::FormatQuantityString(dataSize_) + " elements";
    }

    virtual void VerifyResults() override
    {
        if( outputData_.size() != expectedOutputData_.size() )
        {
            throw std::runtime_error( ( boost::format( "Result verification has failed for fixture \"%1%\". "
                "Output data count is another from expected one." ) ).str() );
        }
        auto mismatchedValues = std::mismatch( outputData_.cbegin(), outputData_.cend(), expectedOutputData_.cbegin(), expectedOutputData_.cend() );
        if( mismatchedValues.first != outputData_.cend() )
        {
            cl_ulong maxAbsError = *mismatchedValues.first - *mismatchedValues.second;
            throw std::runtime_error( ( boost::format( "Result verification has failed for fixture \"%1%\". "
                "Maximum absolute error is %2% for input value %3% "
                "(exact equality is expected)" ) %
                Description() % maxAbsError % *mismatchedValues.first ).str() );
        }
    }

    virtual ~TrivialFactorialFixture()
    {
    }
private:
    const int dataSize_;
    std::vector<int> inputData_;
    std::vector<cl_ulong> expectedOutputData_;
    static const std::unordered_map<int, cl_ulong> TrivialFactorialFixture::correctFactorialValues_;
    std::vector<cl_ulong> outputData_;
    I inputDataIter_;

    void GenerateData()
    {
        inputData_.resize(dataSize_);
        std::copy_n( inputDataIter_, dataSize_, inputData_.begin() );

        // Verify that all input values are in range [0, 20]
        EXCEPTION_ASSERT(std::all_of( inputData_.begin(), inputData_.end(), [] (int i) { return i >= 0 && i <= 20; } ));

        expectedOutputData_.reserve(dataSize_);
        std::transform( inputData_.begin(), inputData_.end(), std::back_inserter(expectedOutputData_), 
            [this] (int i)
            {
                return correctFactorialValues_.at(i);
            } );
    }
};

template<typename I>
const std::unordered_map<int, cl_ulong> TrivialFactorialFixture<I>::correctFactorialValues_ = {
    {0, 1ull},
    {1,  1ull},
    {2,  2ull},
    {3,  6ull},
    {4,  24ull},
    {5,  120ull},
    {6,  720ull},
    {7,  5040ull},
    {8,  40320ull},
    {9,  362880ull},
    {10,  3628800ull},
    {11,  39916800ull},
    {12,  479001600ull},
    {13,  6227020800ull},
    {14,  87178291200ull},
    {15,  1307674368000ull},
    {16,  20922789888000ull},
    {17,  355687428096000ull},
    {18,  6402373705728000ull},
    {19,  121645100408832000ull},
    {20,  2432902008176640000ull},
};
