#include <vector>

#include "catch/single_include/catch.hpp"
#include "program_source_repository.h"
#include "utils.h"

namespace {
// TODO use the same source for unit tests and main application
constexpr const char* const kSource = R"(
__kernel void Pow2ForIntKernel(__global int* input, __global int* output)
{
    size_t id = get_global_id(0);
    output[id] = Pow2ForInt(input[id]);
}
)";
}  // namespace

TEST_CASE("Pow2ForInt works correctly", "[OpenCL math]") {
    std::vector<int32_t> input_values = {-1000, -1, 0, 1, 2, 3, 10};
    std::vector<int32_t> expected_output = {0, 0, 1, 2, 4, 8, 1024};

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

    boost::compute::kernel kernel = Utils::BuildKernel(
        "Pow2ForIntKernel", context,
        Utils::CombineStrings({ProgramSourceRepository::GetOpenCLMathSource(), kSource}));
    kernel.set_arg(0, input_device_vector);
    kernel.set_arg(1, output_device_vector);
    queue.enqueue_1d_range_kernel(kernel, 0, input_values.size(), 0).wait();

    // create vector on host
    std::vector<int> results(input_values.size());

    // copy data back to host
    boost::compute::copy(
        output_device_vector.begin(), output_device_vector.end(), results.begin(), queue);

    CHECK(expected_output == results);
}
