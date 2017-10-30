#include "catch/single_include/catch.hpp"

#include <vector>

#include "program_source_repository.h"
#include "utils.h"

namespace
{
	// TODO use the same source for unit tests and main application
	const char* source = R"(
__kernel void Pow2ForIntKernel(__global int* input, __global int* output)
{
	size_t id = get_global_id(0);
	output[id] = Pow2ForInt(input[id]);
}
)";
}

TEST_CASE( "Pow2ForInt works correctly", "[OpenCL math]" ) {
    std::vector<int32_t> inputValues = { -1000, -1, 0, 1, 2, 3, 10 };
    std::vector<int32_t> expectedOutput = {0, 0, 1, 2, 4, 8, 1024 };

    REQUIRE( inputValues.size() == expectedOutput.size() );
    // get default device and setup context
    boost::compute::device device = boost::compute::system::default_device();
    boost::compute::context context( device );
    boost::compute::command_queue queue( context, device );

    // create vector on device
    boost::compute::vector<int> input_device_vector( inputValues.size(), context );
    boost::compute::vector<int> output_device_vector( inputValues.size(), context );

    // copy from host to device
    boost::compute::copy(
        inputValues.cbegin(), inputValues.cend(), input_device_vector.begin(), queue
    );

    boost::compute::kernel kernel = Utils::BuildKernel( "Pow2ForIntKernel", 
        context,
        Utils::CombineStrings( {ProgramSourceRepository::GetOpenCLMathSource(), source } ) );
    kernel.set_arg( 0, input_device_vector );
    kernel.set_arg( 1, output_device_vector );
    queue.enqueue_1d_range_kernel( kernel, 0, inputValues.size(), 0 );

    // create vector on host
    std::vector<int> results( inputValues.size() );

    // copy data back to host
    boost::compute::copy(
        output_device_vector.begin(), output_device_vector.end(), results.begin(), queue
    );

    REQUIRE( expectedOutput == results );
}