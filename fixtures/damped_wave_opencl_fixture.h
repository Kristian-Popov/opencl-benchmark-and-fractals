#pragma once

#include "boost/compute.hpp"
#include "devices/opencl_device.h"
#include "fixtures/fixture.h"
#include "half_precision_fp.h"
#include "iterators/data_source_adaptor.h"

// TODO get rid of this - non-portable
//#pragma pack(push, 1)
template <typename T>
struct DampedWaveFixtureParameters {
    T amplitude = 0;
    T damping_ratio = 0;
    T angular_frequency = 0;
    T phase = 0;
    T shift = 0;

    DampedWaveFixtureParameters(
        T _amplitude, T _damping_ratio, T _angular_frequency, T _phase, T _shift)
        : amplitude(_amplitude),
          damping_ratio(_damping_ratio),
          angular_frequency(_angular_frequency),
          phase(_phase),
          shift(_shift) {}
};
//#pragma pack(pop)

/*
param_set.amplitude * exp(-param_set.damping_ratio * t) *
cos(param_set.angular_frequency * t + param_set.phase);
Damped wave fixture
( A1 * exp(-D1 * t) * cos( F1 * t + P1 ) ) + ...
Note that this fixture doesn't verify results since we may get very different
results in edge values between CPU and OpenCL implementation (it's even
compiler dependent).
*/
template <typename T>
class DampedWaveOpenClFixture : public Fixture {
public:
    DampedWaveOpenClFixture(
        const std::shared_ptr<OpenClDevice>& device,
        const std::vector<DampedWaveFixtureParameters<T>>& params,
        const std::shared_ptr<DataSource<T>>& input_data_source, size_t data_size,
        const std::string& fixture_name);

    void Initialize() override;

    std::vector<std::string> GetRequiredExtensions() override;

    std::unordered_map<std::string, Duration> Execute() override;

    void StoreResults() override;

    std::shared_ptr<DeviceInterface> Device() override { return device_; }

private:
    typedef DampedWaveFixtureParameters<T> Parameters;

    const std::shared_ptr<OpenClDevice> device_;
    std::vector<T> input_data_;
    std::vector<T> output_data_;
    std::vector<Parameters> params_;
    std::shared_ptr<DataSource<T>> input_data_source_;
    size_t data_size_;
    boost::compute::kernel kernel_;
    std::string fixture_name_;

    void GenerateData();
    /*
    Return results in the following form:
    {
    { xmin, y_1 },
    { x2, y_2 },
    ...
    { xmax, y_n },
    }
    First column contains input value,
    second one - value calculated by OpenCL device
    */
    std::vector<std::vector<T>> GetResults();
};

// Forward declarations of possible variations of this class so we can move implementation
// to a source file
template class DampedWaveOpenClFixture<float>;
template class DampedWaveOpenClFixture<double>;
template class DampedWaveOpenClFixture<half_float::half>;
