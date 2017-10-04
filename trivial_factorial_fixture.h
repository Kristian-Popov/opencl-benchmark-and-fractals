#pragma once

#include <memory>
#include <random>

#include "utils.h"
#include "fixture.h"

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
}
#if 0
ulong FactorialImplementation(ulong val)
{
    ulong result = 1;
    for(int i = 1; i <= val; i++)
    {
        result *= i;
    }
    return result;
}

__kernel void Factorial(__global ulong* input, __global ulong* output)
{
    size_t id = get_global_id(0);
    output[id] = FactorialImplementation(input[id]);
}
#endif

class TrivialFactorialFixture: public Fixture
{
public:
    TrivialFactorialFixture( int dataSize )
        : dataSize_(dataSize)
	{
	}

    virtual void Init() override 
    {
        GenerateData();
    }

    std::unordered_map<OperationId, ExecutionResult> Execute( boost::compute::context& context ) override
	{
        std::unordered_map<OperationId, ExecutionResult> result;

        EXCEPTION_ASSERT( context.get_devices().size() == 1 );
        // create command queue with profiling enabled
        boost::compute::command_queue queue(
            context, context.get_device(), boost::compute::command_queue::enable_profiling
        );

        std::unordered_map<OperationId, boost::compute::event> events;

        // create a vector on the device
        boost::compute::vector<int> input_device_vector( dataSize_, context );

        // copy data from the host to the device
        events.insert( { OperationId::CopyInputDataToDevice, 
            boost::compute::copy_async( inputData_.begin(), inputData_.end(), input_device_vector.begin(), queue ).get_event() 
        } );

        boost::compute::kernel kernel = boost::compute::kernel::create_with_source( trivialFactorialKernelCode, "TrivialFactorial", context );
        boost::compute::vector<unsigned long long> output_device_vector( dataSize_, context );
        kernel.set_arg( 0, input_device_vector );
        kernel.set_arg( 1, output_device_vector );

        events.insert( { OperationId::Calculate,
            queue.enqueue_1d_range_kernel( kernel, 0, dataSize_, 0 )
        } );

        outputData_.resize( dataSize_ );
        boost::compute::event lastEvent = boost::compute::copy_async( output_device_vector.begin(), output_device_vector.end(), outputData_.begin(), queue ).get_event();
        events.insert( { OperationId::CopyOutputDataFromDevice, lastEvent } );

        lastEvent.wait();

        for ( const std::pair<OperationId, boost::compute::event>& v: events )
        {
            result.insert( std::make_pair( v.first, ExecutionResult( v.first, v.second.duration<Duration>() ) ) );
        }

        VerifyOutput();
        return result;
	}

    std::string Description() override
    {
        return std::string("Trivial factorial, ") + std::to_string(dataSize_) + " elements";
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

    void VerifyOutput()
    {
        if( outputData_ != expectedOutputData_ )
        {
            std::string message = "TrivialFactorialFixture: results are different from expected. Expected: \n" +
                Utils::VectorToString( expectedOutputData_ ) + "\n" +
                "Got:\n" +
                Utils::VectorToString( outputData_ );
            throw std::runtime_error( message );
        }
    }

    void GenerateData()
    {
        inputData_.resize(dataSize_);
        // Specify the engine and distribution.
        std::mt19937 mersenne_engine;
        std::uniform_int_distribution<int> dist( 0, 20 );

        auto gen = std::bind( dist, mersenne_engine );
        std::generate( begin( inputData_ ), end( inputData_ ), gen );

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

const std::unordered_map<int, cl_ulong> TrivialFactorialFixture::correctFactorialValues_ = {
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
