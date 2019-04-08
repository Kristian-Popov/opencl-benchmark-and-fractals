#pragma once

#include <complex>
#include <memory>

#include "boost/compute.hpp"

#include <utils/half_precision_fp.h>
#include <utils/utils.h> // TODO split that header to move EXCEPTION_ASSERT to smaller header

// TODO add support for color and grayscale result, both 8 and 16 bit
// may be even floating point pixel formal
// T is temporary value type (must be a floating pointing type)
// P is a pixel type
template<typename T, typename P>
class MultibrotOpenClCalculator
{
public:
    MultibrotOpenClCalculator(
        const boost::compute::device& device,
        const boost::compute::context& context,
        size_t max_width_pix, size_t max_height_pix,
        double power,
        int max_iterations
    );

    // Calculate the given region of Multibrot set.
    // This method only enqueues commands, result will be written to output_iter.
    // Operation is considered to be finished when returned future is ready.
    // This method must not be called before previous operation is finished.
    template<typename I>
    boost::compute::future<I> Calculate(
        std::complex<double> input_min,
        std::complex<double> input_max,
        size_t width_pix, size_t height_pix,
        I output_iter,
        boost::compute::event* calc_event
    )
    {
        EXCEPTION_ASSERT( width_pix <= max_width_pix_ );
        EXCEPTION_ASSERT( height_pix <= max_height_pix_ );

        // Verify that previous operation has finished
        // This operation may cause blocking on some OpenCL implementations.
        // TODO may be if previous operation has not completed yet, log a warning and wait for it?
        if( prev_event_ && prev_event_.status() != CL_COMPLETE )
        {
            throw std::logic_error( "Previous request to calculate and transfer Multibrot data is not yet finished" );
        }

        auto input_min_conv = static_cast<std::complex<T>>( input_min );
        auto input_max_conv = static_cast<std::complex<T>>( input_max );

        std::complex<T> input_diff(
            static_cast<T>( ( input_max_conv.real() - input_min_conv.real() ) / width_pix ),
            static_cast<T>( ( input_max_conv.imag() - input_min_conv.imag() ) / height_pix )
        );
        if( input_diff.real() == 0 || input_diff.imag() == 0 )
        {
            throw std::invalid_argument( "Requested image is too big for current temporary data type." );
        }

        // TODO we could use set_arg() overload for fundamental types for float and double
        // types that is easier to use, but have to add a custom overload for half and its vectors
        kernel_.set_arg( 0, sizeof( T ), &input_min_conv );
        kernel_.set_arg( 1, sizeof( T ), &input_max_conv );
        kernel_.set_arg( 2, sizeof( T ), reinterpret_cast<T(&)[2]>( input_min_conv ) + 1 );
        kernel_.set_arg( 3, sizeof( T ), reinterpret_cast<T(&)[2]>( input_max_conv ) + 1 );
        T power_conv = static_cast<T>( power_ );
        kernel_.set_arg( 4, sizeof( T ), &power_conv );
        kernel_.set_arg( 5, output_device_vector_.get_buffer() );

        boost::compute::extents<2> workgroup_size = {width_pix, height_pix};
        // In-order queue used, so no need to serialize explicitly
        boost::compute::event calculate_event =
            queue_.enqueue_nd_range_kernel( kernel_, 2, nullptr, workgroup_size.data(), nullptr );

        if( calc_event != nullptr )
        {
            *calc_event = calculate_event;
        }

        auto copy_future =
            boost::compute::copy_async( output_device_vector_.cbegin(),
                output_device_vector_.cbegin() + ( width_pix * height_pix ),
                output_iter, queue_ );
        prev_event_ = copy_future.get_event();

        return copy_future;
    }
private:
    std::string SelectPowerFunction();

    boost::compute::device device_;
    boost::compute::context context_;
    boost::compute::command_queue queue_;
    size_t max_width_pix_;
    size_t max_height_pix_;
    double power_;
    int pixel_bit_depth_;
    int max_iterations_;
    boost::compute::kernel kernel_;
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