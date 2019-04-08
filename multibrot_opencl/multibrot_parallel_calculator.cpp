#pragma once

#include "multibrot_parallel_calculator.h"

#include <chrono>

#include <boost/compute.hpp>

#include "utils/utils.h"

const Duration MultibrotParallelCalculator::target_execution_time_ = Duration( std::chrono::seconds( 1 ) );

MultibrotParallelCalculator::MultibrotParallelCalculator(
    size_t width_pix, size_t height_pix,
    std::complex<double> input_min,
    std::complex<double> input_max,
    double power,
    int max_iterations )
    : partitioner_( width_pix, height_pix, fragment_width_pix_, fragment_height_pix_ )
    , input_min_( input_min )
    , input_max_( input_max )
    , width_pix_( width_pix )
    , height_pix_( height_pix )
{
    std::vector<boost::compute::device> devices;
    {
        std::vector<boost::compute::device> cpus, gpus;
        for( const auto& platform : boost::compute::system::platforms() )
        {
            if( use_cpus_ )
            {
                Utils::AppendVectorToVector( cpus, platform.devices( CL_DEVICE_TYPE_CPU ) );
            }
            if( use_gpus_ )
            {
                Utils::AppendVectorToVector( gpus, platform.devices( CL_DEVICE_TYPE_GPU ) );
            }
        }
        if( !cpus.empty() ) // We have at least one CPU device
        {
            if( reserve_1_cpu_thread_ )
            {
                // Split device into two - the first one contains all minus one compute units,
                // it is then added to devices list
                std::vector<boost::compute::device> split_cpu = cpus[0].partition_by_counts(
                    { cpus[0].compute_units() - 1, 1 }
                );
                EXCEPTION_ASSERT( split_cpu.size() == 2 );
                devices.push_back( split_cpu[0] );
            }
            else
            {
                devices.push_back( cpus[0] ); // Add first CPU device as is
            }
            // Add all other CPU devices
            devices.insert( devices.end(), cpus.begin() + 1, cpus.end() );
        }
        // Add all GPUs
        Utils::AppendVectorToVector( devices, gpus );
    }

    for( auto& device: devices )
    {
        // TODO implement automatic resizing of memory buffers?
        device_states_.emplace( device, DeviceState( device, power, max_iterations ) );
    }
}

std::complex<double> MultibrotParallelCalculator::CalcComplexVal( size_t x, size_t y )
{
    return input_min_ + std::complex<double>{
        ( input_max_.real() - input_min_.real() ) * x / width_pix_,
        ( input_max_.imag() - input_min_.imag() ) * y / height_pix_,
    };
}

void MultibrotParallelCalculator::Calculate( Callback cb )
{
    partitioner_.Reset();

    CalculateFirstPhase( cb );

    // Process all remaining operations
    for( auto& device_state : device_states_ )
    {
        if( device_state.second.prev_operation_info )
        {
            ProcessOperationResults( device_state, cb );
        }
    }
}

void MultibrotParallelCalculator::CalculateFirstPhase( Callback cb )
{
    // TODO implement some more reliable check to prevent infinite loops?
    while( 1 )
    {
        for( auto& device_state : device_states_ )
        {
            if( device_state.second.prev_operation_info )
            {
                if( device_state.second.prev_operation_info.value().copy_event.status() ==
                    CL_COMPLETE )
                {
                    ProcessOperationResults( device_state, cb );
                }
            }
            else
            {
                size_t fragment_count = starting_fragment_count_;
                if( device_state.second.prev_operations_duration_sum > Duration() )
                {
                    fragment_count =
                        ( device_state.second.processed_pixels / fragment_size_pix_ ) *
                        ( target_execution_time_ / device_state.second.prev_operations_duration_sum );
                    fragment_count = std::min( fragment_count, max_segment_size_pix_ / fragment_size_pix_ );
                }
                ImagePartitioner::Segment segment = partitioner_.Partition( fragment_count );
                if( segment.IsEmpty() )
                {
                    // No more work to assign, first phase is finished.
                    return;
                }

                device_state.second.prev_operation_info = PrevOperationInfo{
                    boost::compute::event(),
                    boost::compute::event(),
                    segment
                };
                std::complex<double> input_min = CalcComplexVal( segment.x, segment.y );
                std::complex<double> input_max = CalcComplexVal(
                    segment.x + segment.width_pix, segment.y + segment.height_pix );
                auto future = device_state.second.calculator.Calculate( input_min, input_max,
                    segment.width_pix, segment.height_pix, device_state.second.output_vector.begin(),
                    &device_state.second.prev_operation_info.value().calc_event );
                device_state.second.prev_operation_info.value().copy_event = future.get_event();
            }
        }
    }
}

void MultibrotParallelCalculator::ProcessOperationResults(
    std::pair<const boost::compute::device, DeviceState>& device_info,
    Callback cb )
{
    PrevOperationInfo& info = device_info.second.prev_operation_info.value();
    // Checking event status is not a synchronization point (OpenCL 2.2. Reference p.196),
    // so wait until it finishes completely
    info.copy_event.wait();

    cb( device_info.first, info.segment, device_info.second.output_vector.data() );

    // Collect statistics for previous operation
    device_info.second.prev_operations_duration_sum +=
        Duration( info.calc_event ) + Duration( info.copy_event );
    device_info.second.processed_pixels += info.segment.width_pix * info.segment.height_pix;
    device_info.second.prev_operation_info = boost::optional<PrevOperationInfo>();
}