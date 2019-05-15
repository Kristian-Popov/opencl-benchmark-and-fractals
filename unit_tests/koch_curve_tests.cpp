#include <vector>

#include "catch/single_include/catch.hpp"
#include "program_source_repository.h"
#include "utils.h"

namespace {
// TODO use the same source for unit tests and main application
constexpr const char* const kSource = R"(
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
    Output should have space for at least 1 value
    Output:
        - 1st value - size of the structure
        - 2nd value - size of the structure including alignment (taken by measuring distance between pointers)
*/
__kernel void VerifyLineStructKernel(__global ulong* output)
{
    Line value = (Line){.coords = {0, 0, 1, 0}, .ids = {0, 0}};
    output[0] = sizeof(value);

    Line lines[2] = { (Line){.coords = {0, 0, 1, 0}, .ids = {0, 0}}, (Line){.coords = {0, 0, 1, 0}, .ids = {0, 0}} };
    output[1] = (uintptr_t)(lines + 1) - (uintptr_t)(lines);
}
)";
}  // namespace

TEST_CASE("CalcLinesNumberForIteration works correctly", "[Koch curve tests]") {
    std::vector<int32_t> input_values = {-1000, -1, 0, 1, 2, 3};
    std::vector<int32_t> expected_output = {0, 0, 1, 4, 16, 64};

    REQUIRE(input_values.size() == expected_output.size());
    // get default device and setup context
    boost::compute::device device = boost::compute::system::default_device();
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);

    // create vector on device
    boost::compute::vector<int> input_device_vector(input_values.size(), context);
    boost::compute::vector<int> output_device_vector(input_values.size(), context);

    // copy from host to device
    boost::compute::copy(
        input_values.cbegin(), input_values.cend(), input_device_vector.begin(), queue);

    // TODO prepare build options in one place instead of duplicating it here and in main fixture?
    boost::compute::kernel kernel = Utils::BuildKernel(
        "CalcLinesNumberForIterationKernel", context,
        Utils::CombineStrings({ProgramSourceRepository::GetKochCurveSource(), kSource}),
        "-DREAL_T_4=float4 -Werror");
    kernel.set_arg(0, input_device_vector);
    kernel.set_arg(1, output_device_vector);
    queue.enqueue_1d_range_kernel(kernel, 0, input_values.size(), 0).wait();

    // create vector on host
    std::vector<int> results(input_values.size());

    // copy data back to host
    boost::compute::copy(
        output_device_vector.begin(), output_device_vector.end(), results.begin(), queue);

    REQUIRE(expected_output == results);
}

TEST_CASE("CalcGlobalId works correctly", "[Koch curve tests]") {
    std::vector<cl_int2> input_values = {{0, 0}, {1, 0}, {1, 1},  {1, 2}, {1, 3},
                                         {2, 0}, {2, 7}, {2, 15}, {3, 0}};
    std::vector<int32_t> expected_output = {0, 1, 2, 3, 4, 5, 12, 20, 21};

    REQUIRE(input_values.size() == expected_output.size());
    // get default device and setup context
    boost::compute::device device = boost::compute::system::default_device();
    boost::compute::context context(device);
    boost::compute::command_queue queue(context, device);

    // create vector on device
    boost::compute::vector<cl_int2> input_device_vector(input_values.size(), context);
    boost::compute::vector<int> output_device_vector(input_values.size(), context);

    // copy from host to device
    boost::compute::copy(
        input_values.cbegin(), input_values.cend(), input_device_vector.begin(), queue);

    boost::compute::kernel kernel = Utils::BuildKernel(
        "CalcGlobalIdKernel", context,
        Utils::CombineStrings({ProgramSourceRepository::GetKochCurveSource(), kSource}),
        "-DREAL_T_4=float4 -Werror");
    kernel.set_arg(0, input_device_vector);
    kernel.set_arg(1, output_device_vector);
    queue.enqueue_1d_range_kernel(kernel, 0, input_values.size(), 0).wait();

    // create vector on host
    std::vector<int> results(input_values.size());

    // copy data back to host
    boost::compute::copy(
        output_device_vector.begin(), output_device_vector.end(), results.begin(), queue);

    REQUIRE(expected_output == results);
}

TEST_CASE("Line structure is aligned correctly", "[Koch curve tests]") {
    size_t resultSize = 2u;
    std::vector<std::string> types = {
        "float4", "double4"};  // TODO do not run double tests on devices that doesn't support it
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for (boost::compute::platform& platform : platforms) {
        for (boost::compute::device& device : platform.devices()) {
            INFO( "Running test case for platform " << platform.name() <<
                " and device " << device.name() );
            boost::compute::context context(device);
            boost::compute::command_queue queue(context, device);

            for (const std::string& type : types) {
                // create vector on device
                boost::compute::vector<uint64_t> output_device_vector(resultSize, context);

                boost::compute::kernel kernel = Utils::BuildKernel(
                    "VerifyLineStructKernel", context,
                    Utils::CombineStrings({ProgramSourceRepository::GetKochCurveSource(), kSource}),
                    "-DREAL_T_4=" + type + " -Werror", {"cl_khr_fp64"});
                kernel.set_arg(0, output_device_vector);
                queue.enqueue_task(kernel).wait();

                // create vector on host
                std::vector<uint64_t> results(resultSize);

                // copy data back to host
                boost::compute::copy(
                    output_device_vector.begin(), output_device_vector.end(), results.begin(),
                    queue);

                CHECK(results.size() == resultSize);
                CHECK(results.at(0) <= 64);
                CHECK(results.at(1) <= 64);
            }
        }
    }
}
