#pragma once

#include "fixtures/damped_wave_opencl_fixture.h"

#include "boost/format.hpp"
#include "documents/csv_document.h"
#include "opencl_type_traits.h"
#include "utils.h"

namespace {
constexpr const char* const kDampedWaveProgramCode = R"(
// Requires definition of REAL_T macro, it should be one of floating point types (either float or double)
// TODO add unit test to check if size of this structure is the same as in host size.
typedef struct Parameters
{
    REAL_T amplitude;
    REAL_T damping_ratio;
    REAL_T angular_frequency;
    REAL_T phase;
    REAL_T shift;
} Parameters;

REAL_T DampedWave2DImplementation(REAL_T x, __global Parameters* params, int params_count)
{
    REAL_T result = 0;
    // TODO copy parameters to faster memory?
    for (int i = 0; i < params_count; ++i)
    {
        Parameters param_set = params[i];
        // TODO why do we need to take absolute value below?
        REAL_T t = x - param_set.shift;
        result += param_set.amplitude * exp(-param_set.damping_ratio * max( t, (REAL_T)0 )) *
            cos(param_set.angular_frequency * t + param_set.phase);
    }
    return result;
}

__kernel void DampedWave2D(
    __global REAL_T* input,
    __global Parameters* params, int params_count,
    __global REAL_T* output)
{
    size_t id = get_global_id(0);
    output[id] = DampedWave2DImplementation(input[id], params, params_count);
}
)";

const char* kCompilerOptions = "-Werror";
}  // namespace

template <typename T>
DampedWaveOpenClFixture<T>::DampedWaveOpenClFixture(
    const std::shared_ptr<OpenClDevice>& device,
    const std::vector<DampedWaveFixtureParameters<T>>& params,
    const std::shared_ptr<DataSource<T>>& input_data_source, size_t data_size,
    const std::string& fixture_name)
    : device_(device),
      params_(params),
      input_data_source_(input_data_source),
      data_size_(data_size),
      fixture_name_(fixture_name) {}

template <typename T>
void DampedWaveOpenClFixture<T>::Initialize() {
    GenerateData();
    std::string compiler_options =
        kCompilerOptions + std::string(" -DREAL_T=") + OpenClTypeTraits<T>::type_name;
    kernel_ = Utils::BuildKernel(
        "DampedWave2D", device_->GetContext(), kDampedWaveProgramCode, compiler_options,
        GetRequiredExtensions());
}

template <typename T>
std::vector<std::string> DampedWaveOpenClFixture<T>::GetRequiredExtensions() {
    return CollectExtensions<T>();
}

template <typename T>
std::unordered_map<std::string, Duration> DampedWaveOpenClFixture<T>::Execute(
    const RuntimeParams& params) {
    boost::compute::context& context = device_->GetContext();
    boost::compute::command_queue& queue = device_->GetQueue();
    std::unordered_map<std::string, boost::compute::event> events;

    // create a vector on the device
    // TODO do not create it every iteration
    boost::compute::vector<T> input_device_vector(input_data_.size(), context);

    // copy data from the host to the device
    events.insert({"Copying input data",
                   boost::compute::copy_async(
                       input_data_.begin(), input_data_.end(), input_device_vector.begin(), queue)
                       .get_event()});

    boost::compute::vector<Parameters> input_params_vector(params_.size(), context);
    events.insert({"Copying parameters",
                   boost::compute::copy_async(
                       params_.begin(), params_.end(), input_params_vector.begin(), queue)
                       .get_event()});

    boost::compute::vector<T> output_device_vector(input_data_.size(), context);

    kernel_.set_arg(0, input_device_vector);
    kernel_.set_arg(1, input_params_vector);
    EXCEPTION_ASSERT(params_.size() <= std::numeric_limits<cl_int>::max());
    kernel_.set_arg(2, static_cast<cl_int>(params_.size()));
    kernel_.set_arg(3, output_device_vector);

    events.insert(
        {"Calculating", queue.enqueue_1d_range_kernel(kernel_, 0, input_data_.size(), 0)});

    output_data_.resize(input_data_.size());
    boost::compute::event last_event =
        boost::compute::copy_async(
            output_device_vector.begin(), output_device_vector.end(), output_data_.begin(), queue)
            .get_event();
    events.insert({"Copying output data", last_event});

    last_event.wait();

    return Utils::GetOpenCLEventDurations(events);
}

template <typename T>
void DampedWaveOpenClFixture<T>::GenerateData() {
    input_data_.resize(data_size_);
    std::copy_n(DataSourceAdaptor<T>{input_data_source_}, data_size_, input_data_.begin());
}

template <typename T>
std::vector<std::vector<T>> DampedWaveOpenClFixture<T>::GetResults() {
    // Verify that output data are not empty to check if fixture was executed
    EXCEPTION_ASSERT(!output_data_.empty());
    EXCEPTION_ASSERT(output_data_.size() == input_data_.size());
    EXCEPTION_ASSERT(output_data_.size() == data_size_);
    std::vector<std::vector<T>> result;
    for (size_t index = 0; index < output_data_.size(); ++index) {
        result.push_back({input_data_.at(index), output_data_.at(index)});
    }
    return result;
}

template <typename T>
void DampedWaveOpenClFixture<T>::StoreResults() {
    const std::string file_name =
        (boost::format("%1%, %2%.csv") % fixture_name_ % device_->Name()).str();
    CsvDocument csv_document(file_name);
    csv_document.AddValues(GetResults());
    csv_document.BuildAndWriteToDisk();
}
