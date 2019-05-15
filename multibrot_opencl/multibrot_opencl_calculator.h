#pragma once

#include <utils/half_precision_fp.h>
#include <utils/utils.h>  // TODO split that header to move EXCEPTION_ASSERT to smaller header

#include <complex>
#include <memory>

#include "boost/compute.hpp"

// TODO add support for color and grayscale result, both 8 and 16 bit
// may be even floating point pixel formal
// T is temporary value type (must be a floating pointing type)
// P is a pixel type
template <typename T, typename P>
class MultibrotOpenClCalculator {
public:
    MultibrotOpenClCalculator(
        const boost::compute::device& device, const boost::compute::context& context,
        size_t max_width_pix, size_t max_height_pix);

    // Calculate the given region of Multibrot set.
    // This method only enqueues commands, result will be written to output_iter.
    // Operation is considered to be finished when returned future is ready.
    // This method must not be called before previous operation is finished.
    template <typename I>
    boost::compute::future<I> Calculate(
        std::complex<double> input_min, std::complex<double> input_max, size_t width_pix,
        size_t height_pix, double power, int max_iterations, I output_iter,
        boost::compute::event* calc_event) {
        ExecutePrecalculateChecks(width_pix, height_pix, max_iterations);

        // Verify that previous operation has finished
        // This operation may cause blocking on some OpenCL implementations.
        // TODO may be if previous operation has not completed yet, log a warning and wait for it?
        if (prev_event_ && prev_event_.status() != CL_COMPLETE) {
            throw std::logic_error(
                "Previous request to calculate and transfer Multibrot data is not yet finished");
        }

        auto input_min_conv = static_cast<std::complex<T>>(input_min);
        auto input_max_conv = static_cast<std::complex<T>>(input_max);

        std::complex<T> input_diff(
            static_cast<T>((input_max_conv.real() - input_min_conv.real()) / width_pix),
            static_cast<T>((input_max_conv.imag() - input_min_conv.imag()) / height_pix));
        if (input_diff.real() == 0 || input_diff.imag() == 0) {
            throw std::invalid_argument(
                "Requested image is too big for current temporary data type.");
        }

        // TODO we could use set_arg() overload for fundamental types for float and double
        // types that is easier to use, but have to add a custom overload for half and its vectors
        boost::compute::kernel& kernel = universal_kernel_;
        auto kernel_iter = specialized_kernels_.find(power);
        if (kernel_iter != specialized_kernels_.end()) {
            kernel = kernel_iter->second;
        }

        kernel.set_arg(0, sizeof(T), &input_min_conv);
        kernel.set_arg(1, sizeof(T), reinterpret_cast<T(&)[2]>(input_min_conv) + 1);
        kernel.set_arg(2, sizeof(T), &input_max_conv);
        kernel.set_arg(3, sizeof(T), reinterpret_cast<T(&)[2]>(input_max_conv) + 1);
        T power_conv = static_cast<T>(power);
        kernel.set_arg(4, sizeof(T), &power_conv);
        kernel.set_arg(5, static_cast<cl_ushort>(max_iterations));
        kernel.set_arg(6, output_device_vector_.get_buffer());

        boost::compute::extents<2> workgroup_size = {width_pix, height_pix};
        // In-order queue used, so no need to serialize explicitly
        boost::compute::event calculate_event =
            queue_.enqueue_nd_range_kernel(kernel, 2, nullptr, workgroup_size.data(), nullptr);

        if (calc_event != nullptr) {
            *calc_event = calculate_event;
        }

        auto copy_future = boost::compute::copy_async(
            output_device_vector_.cbegin(),
            output_device_vector_.cbegin() + (width_pix * height_pix), output_iter, queue_);
        prev_event_ = copy_future.get_event();

        return copy_future;
    }

private:
    void BuildKernels();
    std::string PrepareCompilerOptions(const std::string& power_func);
    void ExecutePrecalculateChecks(size_t width_pix, size_t height_pix, int max_iterations);

    boost::compute::device device_;
    boost::compute::context context_;
    boost::compute::command_queue queue_;
    size_t max_width_pix_;
    size_t max_height_pix_;
    int pixel_bit_depth_;
    std::unordered_map<double /* power */, boost::compute::kernel> specialized_kernels_;
    boost::compute::kernel universal_kernel_;
    boost::compute::vector<P> output_device_vector_;
    boost::compute::event prev_event_;
};

// Grayscale 8 bit
template class MultibrotOpenClCalculator<half_float::half, cl_uchar>;
template class MultibrotOpenClCalculator<float, cl_uchar>;
template class MultibrotOpenClCalculator<double, cl_uchar>;

// Grayscale 16 bit
template class MultibrotOpenClCalculator<half_float::half, cl_ushort>;
template class MultibrotOpenClCalculator<float, cl_ushort>;
template class MultibrotOpenClCalculator<double, cl_ushort>;

// RGB 8 bit
template class MultibrotOpenClCalculator<half_float::half, cl_uchar4>;
template class MultibrotOpenClCalculator<float, cl_uchar4>;
template class MultibrotOpenClCalculator<double, cl_uchar4>;

// RGB 16 bit
template class MultibrotOpenClCalculator<half_float::half, cl_ushort4>;
template class MultibrotOpenClCalculator<float, cl_ushort4>;
template class MultibrotOpenClCalculator<double, cl_ushort4>;
