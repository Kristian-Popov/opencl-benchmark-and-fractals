#pragma once

#include <complex>
#include <memory>

#include "boost/compute.hpp"
#include "devices/opencl_device.h"
#include "fixtures/fixture.h"
#include "multibrot_opencl/multibrot_opencl_calculator.h"

// TODO add support for color and grayscale result, both 8 and 16 bit
// may be even floating point pixel formal
// T is pixel type
template <typename T, typename P>
class MultibrotOpenClFixture : public Fixture {
public:
    MultibrotOpenClFixture(
        const std::shared_ptr<OpenClDevice>& device, size_t width_pix, size_t height_pix,
        std::complex<double> input_min, std::complex<double> input_max, double power,
        const std::string& fixture_name);

    void Initialize() override;

    std::vector<std::string> GetRequiredExtensions() override;

    std::unordered_map<std::string, Duration> Execute() override;

    void StoreResults() override;

    std::shared_ptr<DeviceInterface> Device() override { return device_; }

private:
    std::shared_ptr<OpenClDevice> device_;
    size_t width_pix_;
    size_t height_pix_;
    std::complex<double> input_min_;
    std::complex<double> input_max_;
    double power_;
    std::string fixture_name_;
    std::unique_ptr<MultibrotOpenClCalculator<T, P>> calculator_;
    std::vector<P> output_data_;
};

// Grayscale 8 bit
template class MultibrotOpenClFixture<half_float::half, cl_uchar>;
template class MultibrotOpenClFixture<float, cl_uchar>;
template class MultibrotOpenClFixture<double, cl_uchar>;

// Grayscale 16 bit
template class MultibrotOpenClFixture<half_float::half, cl_ushort>;
template class MultibrotOpenClFixture<float, cl_ushort>;
template class MultibrotOpenClFixture<double, cl_ushort>;
