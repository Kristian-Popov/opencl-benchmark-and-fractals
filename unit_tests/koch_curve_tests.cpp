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
    queue.enqueue_1d_range_kernel( kernel, 0, inputValues.size(), 0 ).wait();

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
    queue.enqueue_1d_range_kernel( kernel, 0, inputValues.size(), 0 ).wait();

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

    REQUIRE( expectedOutput.at(0) == sizeof( inputValue ) );
    REQUIRE( expectedOutput.at( 1 ) == sizeof( inputValue.coords ) );
    REQUIRE( expectedOutput.at( 2 ) == sizeof( inputValue.ids ) );

    uintptr_t ptr = (uintptr_t) ( &inputValue );
    uintptr_t coordsPtr = (uintptr_t) ( &( inputValue.coords ) );
    uintptr_t idsPtr = (uintptr_t) ( &( inputValue.ids ) );

    REQUIRE( expectedOutput.at( 3 ) == coordsPtr - ptr );
    REQUIRE( expectedOutput.at( 4 ) == idsPtr - ptr );

    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for ( boost::compute::device& device: platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
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
            queue.enqueue_task( kernel ).wait();

            // create vector on host
            std::vector<uint64_t> results( expectedOutput.size() );

            // copy data back to host
            boost::compute::copy(
                output_device_vector.begin(), output_device_vector.end(), results.begin(), queue
            );

            REQUIRE( expectedOutput == results );
        }
    }
}

TEST_CASE( "KochSnowflakeKernel can be built and run", "[Koch curve tests]" ) {
    const char* source = R"(
REAL_T_4 KochSnowflakeCalc(__global Line* line, REAL_T_4 transformMatrix, REAL_T_2 offset)
{
    REAL_T_2 start = line->coords.lo;
	REAL_T_2 end = line->coords.hi;
    REAL_T_2 newStart = MultiplyMatrix2x2AndVector(transformMatrix, start) + offset;
    REAL_T_2 newEnd = MultiplyMatrix2x2AndVector(transformMatrix, end) + offset;
    return (REAL_T_4)(newStart, newEnd);
}
/*
    Build a Koch's snowflake or another figure based on Koch's curves.
    This kernel takes an array of lines ("input", e.g. result of "KochCurveCalcKernel")
    as a data source and "curve" (where curve starts and stops).
    Source data is transformed (e.g. scaled, rotated and moved) accoding to "curve".
    
    Every curve is just a general Koch curve, but combining them you can build
    different figures like Koch snowflakes.

    Result data is stored in "output" starting at "outputOffset".
    "outputOffset" allows to enqueue few kernels for different curves
    to form one figure.
*/
__kernel void KochSnowflakeKernel(__global Line* input, REAL_T_4 curve,
    __global REAL_T_4* output, int outputOffset)
{
    size_t threadId = get_global_id(0);

    REAL_T_2 curveVector = curve.hi - curve.lo;
    REAL_T_2 curveVectorNorm = normalize(curveVector);
    REAL_T_4 transformMatrix = length(curveVector) * 
        (REAL_T_4)(curveVectorNorm.x, -curveVectorNorm.y, curveVectorNorm.y, curveVectorNorm.x);
    REAL_T_2 offset = curve.lo;

    size_t resultId = outputOffset + threadId;
    output[resultId] = KochSnowflakeCalc(input + threadId, transformMatrix, offset);
}
)";

    typedef KochCurveUtils::Line<cl_float4> Line;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            // create vector on device
            size_t dataSize = 10;
            std::vector<Line> inputData( dataSize );
            for (int i = 0; i < dataSize; ++i)
            {
                inputData.at(i) = Line( {100.0f*i, 200.0f*i, 300.0f*i, 400.0f*i}, {0, 0});
            }
            boost::compute::vector<KochCurveUtils::Line<cl_float4>> inputDeviceVector( dataSize, context );
            boost::compute::copy( inputData.cbegin(), inputData.cend(), inputDeviceVector.begin(), queue );

            boost::compute::vector<cl_float4> outputDeviceVector( dataSize, context );

            boost::compute::kernel kernel;
            CHECK_NOTHROW( kernel = Utils::BuildKernel( "KochSnowflakeKernel",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetKochCurveSource(), source} ),
                "-DREAL_T_2=float2 -DREAL_T_4=float4 -Werror" ) );
            kernel.set_arg( 0, inputDeviceVector );
            kernel.set_arg( 1, cl_float4( { 0.0f, 0.0f, 1.0f, 0.0f } ) ); // This curve should leave data as is
            kernel.set_arg( 2, outputDeviceVector );
            kernel.set_arg( 3, 0 );
            CHECK_NOTHROW( queue.enqueue_1d_range_kernel( kernel, 0, dataSize, 0 ).wait() );

            // create vector on host
            std::vector<cl_float4> results( dataSize );

            // copy data back to host
            boost::compute::copy(
                outputDeviceVector.begin(), outputDeviceVector.end(), results.begin(), queue
            );

            if ( !std::equal( results.cbegin(), results.cend(), inputData.cbegin(),
                [] (const cl_float4& lhs, const Line& rhs)
                {
                    return lhs.x == rhs.coords.x &&
                        lhs.y == rhs.coords.y &&
                        lhs.z == rhs.coords.z &&
                        lhs.w == rhs.coords.w;
                } ) )
            {
                FAIL_CHECK( "KochSnowflakeKernel output is different from expected one." );
            }
        }
    }
}