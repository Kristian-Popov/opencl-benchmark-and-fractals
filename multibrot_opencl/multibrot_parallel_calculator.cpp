#pragma once

#include "multibrot_parallel_calculator.h"

#include <boost/compute.hpp>
#include <chrono>

#include "utils/utils.h"

template <typename P>
const Duration MultibrotParallelCalculator<P>::target_execution_time_ =
    Duration(std::chrono::seconds(1));

template <typename P>
MultibrotParallelCalculator<P>::MultibrotParallelCalculator(size_t width_pix, size_t height_pix)
    : partitioner_(width_pix, height_pix, fragment_width_pix_, fragment_height_pix_),
      width_pix_(width_pix),
      height_pix_(height_pix) {
    std::vector<boost::compute::device> devices;
    {
        std::vector<boost::compute::device> cpus, gpus;
        for (const auto& platform : boost::compute::system::platforms()) {
            if (use_cpus_) {
                Utils::AppendVectorToVector(cpus, platform.devices(CL_DEVICE_TYPE_CPU));
            }
            if (use_gpus_) {
                Utils::AppendVectorToVector(gpus, platform.devices(CL_DEVICE_TYPE_GPU));
            }
        }
        if (!cpus.empty())  // We have at least one CPU device
        {
            if (reserve_1_cpu_thread_) {
                // Split device into two - the first one contains all minus one compute units,
                // it is then added to devices list
                std::vector<boost::compute::device> split_cpu =
                    cpus[0].partition_by_counts({cpus[0].compute_units() - 1, 1});
                EXCEPTION_ASSERT(split_cpu.size() == 2);
                devices.push_back(split_cpu[0]);
            } else {
                devices.push_back(cpus[0]);  // Add first CPU device as is
            }
            // Add all other CPU devices
            devices.insert(devices.end(), cpus.begin() + 1, cpus.end());
        }
        // Add all GPUs
        Utils::AppendVectorToVector(devices, gpus);
    }

    for (auto& device : devices) {
        // TODO implement automatic resizing of memory buffers?
        device_states_.emplace(device, DeviceState(device));
    }
}

template <typename P>
std::complex<double> MultibrotParallelCalculator<P>::CalcComplexVal(
    std::complex<double> input_min, std::complex<double> input_max, size_t x, size_t y) {
    return input_min + std::complex<double>{
        (input_max.real() - input_min.real()) * x / width_pix_,
        (input_max.imag() - input_min.imag()) * y / height_pix_,
    };
}

template <typename P>
void MultibrotParallelCalculator<P>::Calculate(
    std::complex<double> input_min, std::complex<double> input_max, double power,
    int max_iterations, Callback cb) {
    partitioner_.Reset();

    CalculateFirstPhase(input_min, input_max, power, max_iterations, cb);

    // Process all remaining operations
    for (auto& device_state : device_states_) {
        if (device_state.second.prev_operation_info) {
            ProcessOperationResults(device_state, cb);
        }
    }
}

template <typename P>
void MultibrotParallelCalculator<P>::CalculateFirstPhase(
    std::complex<double> input_min, std::complex<double> input_max, double power,
    int max_iterations, Callback cb) {
    // TODO implement some more reliable check to prevent infinite loops?
    while (1) {
        for (auto& device_state : device_states_) {
            if (device_state.second.prev_operation_info) {
                if (device_state.second.prev_operation_info.value().copy_event.status() ==
                    CL_COMPLETE) {
                    ProcessOperationResults(device_state, cb);
                }
            } else {
                size_t fragment_count = starting_fragment_count_;
                if (device_state.second.prev_operations_duration_sum > Duration()) {
                    fragment_count =
                        (device_state.second.processed_pixels / fragment_size_pix_) *
                        (target_execution_time_ / device_state.second.prev_operations_duration_sum);
                    fragment_count =
                        std::min(fragment_count, max_segment_size_pix_ / fragment_size_pix_);
                }
                ImagePartitioner::Segment segment = partitioner_.Partition(fragment_count);
                if (segment.IsEmpty()) {
                    // No more work to assign, first phase is finished.
                    return;
                }

                device_state.second.prev_operation_info =
                    PrevOperationInfo{boost::compute::event(), boost::compute::event(), segment};
                std::complex<double> min =
                    CalcComplexVal(input_min, input_max, segment.x, segment.y);
                std::complex<double> max = CalcComplexVal(
                    input_min, input_max, segment.x + segment.width_pix,
                    segment.y + segment.height_pix);
                auto future = device_state.second.calculator.Calculate(
                    min, max, segment.width_pix, segment.height_pix, power, max_iterations,
                    device_state.second.output_vector.begin(),
                    &device_state.second.prev_operation_info.value().calc_event);
                device_state.second.prev_operation_info.value().copy_event = future.get_event();
            }
        }
    }
}

template <typename P>
void MultibrotParallelCalculator<P>::ProcessOperationResults(
    std::pair<const boost::compute::device, DeviceState>& device_info, Callback cb) {
    PrevOperationInfo& info = device_info.second.prev_operation_info.value();
    // Checking event status is not a synchronization point (OpenCL 2.2. Reference p.196),
    // so wait until it finishes completely
    info.copy_event.wait();

    cb(device_info.first, info.segment, device_info.second.output_vector.data());

    // Collect statistics for previous operation
    device_info.second.prev_operations_duration_sum +=
        Duration(info.calc_event) + Duration(info.copy_event);
    device_info.second.processed_pixels += info.segment.width_pix * info.segment.height_pix;
    device_info.second.prev_operation_info = boost::optional<PrevOperationInfo>();
}
