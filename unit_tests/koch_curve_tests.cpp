#include "catch/single_include/catch.hpp"

#include <vector>

#include "koch_curve_utils.h"
#include "program_source_repository.h"
#include "utils.h"

namespace
{
	// TODO use the same source for unit tests and main application
	const char* source = R"(
__kernel void CalcLinesNumberForIterationKernel(__global int* input, __global int* output)
{
	size_t id = get_global_id(0);
	output[id] = CalcLinesNumberForIteration(input[id]);
}

__kernel void CalcGlobalIdKernel(__global int2* input, __global int* output)
{
	size_t id = get_global_id(0);
	output[id] = CalcGlobalId(input[id]);
}

/* Verify size and alignment of all fields in structure Line
    Should be run on one thread only.
    Output should have space for at least 5 values
    Output:
        - 1st value - size of the structure
        - 2nd value - size of the field "coords"
        - 3rd value - size of the field "ids"
        - 4th value - offset of the field "coords"
        - 5th value - offset of the field "ids"
*/
__kernel void VerifyLineStructKernel(Line value, __global ulong* output)
{
	output[0] = sizeof(value);
    output[1] = sizeof(value.coords);
    output[2] = sizeof(value.ids);

    uintptr_t ptr = (uintptr_t)(&value);
    uintptr_t coordsPtr = (uintptr_t)(&(value.coords));
    uintptr_t idsPtr = (uintptr_t)(&(value.ids));

    output[3] = coordsPtr - ptr;
    output[4] = idsPtr - ptr;
}
)";
}

TEST_CASE( "CalcLinesNumberForIteration works correctly", "[Koch curve tests]" ) {
    std::vector<int32_t> inputValues = { -1000, -1, 0, 1, 2, 3 };
    std::vector<int32_t> expectedOutput = {0, 0, 1, 4, 16, 64 };

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

    // TODO prepare build options in one place instead of duplicating it here and in main fixture?
    boost::compute::kernel kernel = Utils::BuildKernel( "CalcLinesNumberForIterationKernel", 
        context,
        Utils::CombineStrings( { ProgramSourceRepository::GetKochCurveSource(), source } ),
        "-DREAL_T_4=float4 -Werror" );
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

TEST_CASE( "CalcGlobalId works correctly", "[Koch curve tests]" ) {
    std::vector<cl_int2> inputValues = { {0, 0}, {1, 0}, {1, 1}, {1, 2}, {1, 3}, {2, 0}, {2, 7},
    {2, 15}, {3, 0}};
    std::vector<int32_t> expectedOutput = {0, 1, 2, 3, 4, 5, 12, 20, 21};

    REQUIRE( inputValues.size() == expectedOutput.size() );
    // get default device and setup context
    boost::compute::device device = boost::compute::system::default_device();
    boost::compute::context context( device );
    boost::compute::command_queue queue( context, device );

    // create vector on device
    boost::compute::vector<cl_int2> input_device_vector( inputValues.size(), context );
    boost::compute::vector<int> output_device_vector( inputValues.size(), context );

    // copy from host to device
    boost::compute::copy(
        inputValues.cbegin(), inputValues.cend(), input_device_vector.begin(), queue
    );

    boost::compute::kernel kernel = Utils::BuildKernel( "CalcGlobalIdKernel",
        context,
        Utils::CombineStrings( {ProgramSourceRepository::GetKochCurveSource(), source} ),
        "-DREAL_T_4=float4 -Werror" );
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

TEST_CASE( "Line structure is aligned correctly for single precision values", "[Koch curve tests]" ) {
    std::vector<uint64_t> expectedOutput = {24, 16, 8, 0, 16};
    KochCurveUtils::Line<cl_float4> inputValue;
    inputValue.coords = { 10.0f, 20.0f, 30.0f, 40.0f };
    inputValue.ids = { 100, 100 };

    // get default device and setup context
    boost::compute::device device = boost::compute::system::default_device();
    boost::compute::context context( device );
    boost::compute::command_queue queue( context, device );

    // create vector on device
    boost::compute::vector<uint64_t> output_device_vector( expectedOutput.size(), context );

    boost::compute::kernel kernel = Utils::BuildKernel( "VerifyLineStructKernel",
        context,
        Utils::CombineStrings( {ProgramSourceRepository::GetKochCurveSource(), source} ),
        "-DREAL_T_4=float4 -Werror" );
    kernel.set_arg( 0, sizeof( inputValue ), &inputValue );
    kernel.set_arg( 1, output_device_vector );
    queue.enqueue_task( kernel );

    // create vector on host
    std::vector<uint64_t> results( expectedOutput.size() );

    // copy data back to host
    boost::compute::copy(
        output_device_vector.begin(), output_device_vector.end(), results.begin(), queue
    );

    REQUIRE( expectedOutput == results );
}