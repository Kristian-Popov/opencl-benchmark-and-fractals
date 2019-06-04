#pragma once

#include <boost/format.hpp>
#include <memory>
#include <random>

#include "boost/compute.hpp"
#include "data_verification_failed_exception.h"
#include "fixtures/fixture.h"
#include "iterators/data_source_adaptor.h"
#include "utils.h"

namespace {
const char* kTrivialFactorialKernelCode = R"(
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

constexpr const char* const kCompilerOptions = "-Werror";
}  // namespace

class TrivialFactorialOpenClFixture : public Fixture {
public:
    TrivialFactorialOpenClFixture(
        const std::shared_ptr<OpenClDevice>& device,
        const std::shared_ptr<DataSource<int>>& input_data_source, int data_size)
        : device_(device), input_data_source_(input_data_source), data_size_(data_size) {}

    virtual void Initialize() override {
        GenerateData();
        kernel_ = Utils::BuildKernel(
            "TrivialFactorial", device_->GetContext(), kTrivialFactorialKernelCode,
            kCompilerOptions);
    }

    virtual std::vector<std::string> GetRequiredExtensions() override {
        return std::vector<std::string>();  // This fixture doesn't require any special extensions
    }

    std::unordered_map<std::string, Duration> Execute(const RuntimeParams& params) override {
        boost::compute::context& context = device_->GetContext();
        boost::compute::command_queue& queue = device_->GetQueue();

        std::unordered_map<std::string, boost::compute::event> events;

        // create a vector on the device
        boost::compute::vector<int> input_device_vector(data_size_, context);

        // copy data from the host to the device
        events.insert({"Copying input data", boost::compute::copy_async(
                                                 input_data_.begin(), input_data_.end(),
                                                 input_device_vector.begin(), queue)
                                                 .get_event()});

        boost::compute::vector<unsigned long long> output_device_vector(data_size_, context);
        kernel_.set_arg(0, input_device_vector);
        kernel_.set_arg(1, output_device_vector);

        events.insert({"Calculating", queue.enqueue_1d_range_kernel(kernel_, 0, data_size_, 0)});

        output_data_.resize(data_size_);
        boost::compute::event last_event =
            boost::compute::copy_async(
                output_device_vector.begin(), output_device_vector.end(), output_data_.begin(),
                queue)
                .get_event();
        events.insert({"Copying output data", last_event});

        last_event.wait();

        return Utils::GetOpenCLEventDurations(events);
    }

    virtual void VerifyResults() override {
        if (output_data_.size() != expected_output_data_.size()) {
            throw std::runtime_error(
                (boost::format("Result verification has failed for fixture \"%1%\". "
                               "Output data count is another from expected one."))
                    .str());
        }
        auto mismatched_values = std::mismatch(
            output_data_.cbegin(), output_data_.cend(), expected_output_data_.cbegin(),
            expected_output_data_.cend());
        if (mismatched_values.first != output_data_.cend()) {
            cl_ulong max_abs_error = *mismatched_values.first - *mismatched_values.second;
            throw DataVerificationFailedException(
                (boost::format("Result verification has failed for trivial factorial fixture. "
                               "Maximum absolute error is %1% for input value %2% "
                               "(exact equality is expected).") %
                 max_abs_error % *mismatched_values.first)
                    .str());
        }
    }

    std::shared_ptr<DeviceInterface> Device() override { return device_; }

    virtual ~TrivialFactorialOpenClFixture() noexcept {}

private:
    const int data_size_;
    std::vector<int> input_data_;
    std::vector<cl_ulong> expected_output_data_;
    static const std::unordered_map<int, cl_ulong>
        TrivialFactorialOpenClFixture::correct_factorial_values_;
    std::vector<cl_ulong> output_data_;
    std::shared_ptr<DataSource<int>> input_data_source_;
    boost::compute::kernel kernel_;
    const std::shared_ptr<OpenClDevice> device_;

    void GenerateData() {
        input_data_.resize(data_size_);
        std::copy_n(DataSourceAdaptor<int>{input_data_source_}, data_size_, input_data_.begin());

        // Verify that all input values are in range [0, 20]
        EXCEPTION_ASSERT(std::all_of(
            input_data_.begin(), input_data_.end(), [](int i) { return i >= 0 && i <= 20; }));

        expected_output_data_.reserve(data_size_);
        std::transform(
            input_data_.begin(), input_data_.end(), std::back_inserter(expected_output_data_),
            [this](int i) { return correct_factorial_values_.at(i); });
    }
};

const std::unordered_map<int, cl_ulong> TrivialFactorialOpenClFixture::correct_factorial_values_ = {
    {0, 1ull},
    {1, 1ull},
    {2, 2ull},
    {3, 6ull},
    {4, 24ull},
    {5, 120ull},
    {6, 720ull},
    {7, 5040ull},
    {8, 40320ull},
    {9, 362880ull},
    {10, 3628800ull},
    {11, 39916800ull},
    {12, 479001600ull},
    {13, 6227020800ull},
    {14, 87178291200ull},
    {15, 1307674368000ull},
    {16, 20922789888000ull},
    {17, 355687428096000ull},
    {18, 6402373705728000ull},
    {19, 121645100408832000ull},
    {20, 2432902008176640000ull},
};
