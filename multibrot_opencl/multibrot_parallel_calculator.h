#pragma once

#include <complex>
#include <unordered_map>

#include <utils/duration.h>
#include "image_partitioner.h"
#include "multibrot_opencl_calculator.h"

namespace std
{
    // TODO move this to some dedicated place?
    template<> struct hash<::boost::compute::device>
    {
        typedef ::boost::compute::device argument_type;
        typedef std::size_t result_type;
        result_type operator()( const argument_type& s ) const noexcept
        {
            // boost::compute::device is a wrapper for cl_device_id, which itself is a pointer
            // which hash is defined
            return std::hash<cl_device_id>()( s.id() );
        }
    };
}

template<typename P>
class MultibrotParallelCalculator
{
public:
    typedef P ResultType;
    typedef std::function<void(
        const boost::compute::device&,
        const ImagePartitioner::Segment&,
        const ResultType*)> Callback;
    MultibrotParallelCalculator(
        size_t width_pix, size_t height_pix
    );

    // TODO how callback should be provided, by value or reference?
    void Calculate(
        std::complex<double> input_min, std::complex<double> input_max,
        double power,
        int max_iterations,
        Callback cb
    );
private:
    typedef float TempValueType;
    typedef MultibrotOpenClCalculator<TempValueType, ResultType> Calculator;

    struct PrevOperationInfo
    {
        boost::compute::event calc_event;
        boost::compute::event copy_event;
        ImagePartitioner::Segment segment;
    };

    struct DeviceState
    {
        Calculator calculator;
        std::vector<ResultType> output_vector;
        Duration prev_operations_duration_sum;
        size_t processed_pixels = 0;
        boost::optional<PrevOperationInfo> prev_operation_info;

        DeviceState( const boost::compute::device& device )
            : calculator( device, boost::compute::context{device},
                max_segment_width_pix_, max_segment_height_pix_ )
            , output_vector( max_segment_width_pix_ * max_segment_height_pix_ )
        {}
    };

    std::complex<double> CalcComplexVal( std::complex<double> input_min, std::complex<double> input_max, size_t x, size_t y );
    void ProcessOperationResults(
        std::pair<const boost::compute::device, DeviceState>& device_info,
        Callback cb );
    void CalculateFirstPhase(
        std::complex<double> input_min, std::complex<double> input_max,
        double power,
        int max_iterations,
        Callback cb
    );

    std::unordered_map<boost::compute::device, DeviceState> device_states_;
    ImagePartitioner partitioner_;
    size_t width_pix_;
    size_t height_pix_;
    static constexpr size_t fragment_width_pix_ = 100;
    static constexpr size_t fragment_height_pix_ = 100;
    static constexpr size_t fragment_size_pix_ = fragment_width_pix_ * fragment_height_pix_;
    static constexpr size_t max_segment_width_pix_ = 10000;
    static constexpr size_t max_segment_height_pix_ = 100;
    static constexpr size_t max_segment_size_pix_ = max_segment_width_pix_ * max_segment_height_pix_;
    static constexpr size_t starting_fragment_count_ = 1;
    static constexpr bool use_cpus_ = true;
    static constexpr bool reserve_1_cpu_thread_ = true;
    static constexpr bool use_gpus_ = true;
    static const Duration target_execution_time_;
};

template class MultibrotParallelCalculator<cl_uchar>;
template class MultibrotParallelCalculator<cl_ushort>;
template class MultibrotParallelCalculator<cl_uchar4>;
template class MultibrotParallelCalculator<cl_ushort4>;