#include "catch/single_include/catch.hpp"

#include <vector>

#include "program_source_repository.h"
#include "utils.h"

namespace
{
    const char* kSource = R"(
/*
    Initialiazes a memory pool and then returns offsets of all assigned memory areas.
    results should hold at least "alloc_count" values
*/
__kernel void VerifyHappyPathMemoryAllocSinglethreaded(__global volatile uchar* mem, uint size, uint alloc_count, uint alloc_size, __global ulong* results)
{
    if (size < alloc_count * alloc_size + sizeof(uint))
    {
        // TODO report error
        return;
    }
    struct GlobalMemoryPool memory_pool;
    InitGlobalMemoryPool(&memory_pool, mem, size);
    __global volatile uchar* firstPtr = AllocateMemory(&memory_pool, alloc_size);
    results[0] = 0;
    for (int i = 1; i < alloc_count; ++i)
    {
        __global volatile uchar* ptr = AllocateMemory(&memory_pool, alloc_size);
        results[i] = (ulong)(ptr - firstPtr);
    }
}

/*
    Initialiazes a memory pool and returns offsets of all assigned memory areas.
    results should hold at least "alloc_count" values
*/
__kernel void VerifyHappyPathMemoryAllocMultithreaded(__global volatile uchar* mem, uint size, uint alloc_size, __global ulong* results)
{
    struct GlobalMemoryPool memory_pool;
    InitGlobalMemoryPool(&memory_pool, mem, size);
    size_t id = get_global_id(0);
    results[id] = (ulong)AllocateMemory(&memory_pool, alloc_size);
}

/*
    Initialiazes a memory pool, then every thread allocates space for 10 uint values and writes values from 1 to 10 to this space.
*/
__kernel void VerifyHappyPathWriteToAllocatedMemoryMultithreaded(__global volatile uchar* mem, uint size, uint alloc_size)
{
    if (alloc_size != 10 * sizeof(uint))
    {
        // TODO return error
        return;
    }
    struct GlobalMemoryPool memory_pool;
    InitGlobalMemoryPool(&memory_pool, mem, size);
    size_t id = get_global_id(0);
    __global volatile uint* data = AllocateMemory(&memory_pool, alloc_size);
    for (uint i = 0; i < 10; ++i)
    {
        data[i] = i + 1;
    }
}
)";
}

TEST_CASE( "Happy path memory allocation from one thread", "[Global memory pool tests]" ) {
    const cl_uint alloc_size = 10;
    const cl_uint alloc_count = 10;
    const cl_uint total_memory_size_ints = 1000;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            boost::compute::vector<cl_int> buf( total_memory_size_ints, 0, queue ); // Memory pool
            boost::compute::vector<cl_ulong> output_device_vector( 10, context );

            boost::compute::kernel kernel = Utils::BuildKernel( "VerifyHappyPathMemoryAllocSinglethreaded",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetGlobalMemoryPoolSource(), kSource} ),
                {},
                {"cl_khr_global_int32_base_atomics"} );
            kernel.set_arg( 0, buf );
            kernel.set_arg( 1, static_cast<cl_uint>( total_memory_size_ints * sizeof( cl_int ) ) );
            kernel.set_arg( 2, alloc_count );
            kernel.set_arg( 3, alloc_size );
            kernel.set_arg( 4, output_device_vector );
            queue.enqueue_task( kernel ).wait();

            // create vector on host
            std::vector<cl_ulong> results( alloc_count );

            // copy data back to host
            boost::compute::copy(
                output_device_vector.begin(), output_device_vector.end(), results.begin(), queue
            );

            std::vector<cl_ulong> expected_output = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
            CHECK( results == expected_output );
        }
    }
}

TEST_CASE( "Happy path memory allocation from multiple threads", "[Global memory pool tests]" ) {
    const cl_uint alloc_size = 10;
    const cl_uint alloc_count = 10;
    const cl_uint total_memory_size_ints = 1000;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            boost::compute::vector<cl_int> buf( total_memory_size_ints, 0, queue ); // Memory pool
            boost::compute::vector<cl_ulong> output_device_vector( 10, context );

            boost::compute::kernel kernel = Utils::BuildKernel( "VerifyHappyPathMemoryAllocMultithreaded",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetGlobalMemoryPoolSource(), kSource} ),
                {},
                {"cl_khr_global_int32_base_atomics"} );
            kernel.set_arg( 0, buf );
            kernel.set_arg( 1, static_cast<cl_uint>( total_memory_size_ints * sizeof( cl_int ) ) );
            kernel.set_arg( 2, alloc_size );
            kernel.set_arg( 3, output_device_vector );
            queue.enqueue_1d_range_kernel( kernel, 0, alloc_count, 0 ).wait();

            // create vector on host
            std::vector<cl_ulong> results( alloc_count );

            // copy data back to host
            boost::compute::copy(
                output_device_vector.begin(), output_device_vector.end(), results.begin(), queue
            );

            // Kernel returned pointers stored as unsigned integers. First thing we should do is to sort them so we have them
            // in ascending order
            std::sort( results.begin(), results.end() );
            // Now remove the first value from all of them (first value is a base memory address, all others will be greater than it
            // and relative to it)
            cl_ulong first_address = results.at( 0 );
            std::transform( results.begin(), results.end(), results.begin(),
                [first_address]( cl_ulong val )
            {
                return val - first_address;
            } );

            std::vector<cl_ulong> expected_output = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
            CHECK( results == expected_output );
        }
    }
}

TEST_CASE( "Write data in multiple threads", "[Global memory pool tests]" ) {
    std::vector<cl_uint> pattern = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const cl_uint alloc_size = pattern.size() * sizeof( cl_uint );
    const cl_uint alloc_count = 10;
    const cl_uint total_memory_size_ints = 1000;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            boost::compute::vector<cl_uint> buf( total_memory_size_ints, 0, queue ); // Memory pool

            boost::compute::kernel kernel = Utils::BuildKernel( "VerifyHappyPathWriteToAllocatedMemoryMultithreaded",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetGlobalMemoryPoolSource(), kSource} ) );

            kernel.set_arg( 0, buf );
            kernel.set_arg( 1, static_cast<cl_uint>( total_memory_size_ints * sizeof( cl_int ) ) );
            kernel.set_arg( 2, alloc_size );
            queue.enqueue_1d_range_kernel( kernel, 0, alloc_count, 0 ).wait();

            // create vector on host
            std::vector<cl_uint> results( alloc_count * pattern.size() );

            // copy data back to host
            boost::compute::copy_n(
                buf.begin() + 1, alloc_count * pattern.size(), results.begin(), queue
            );

            std::vector<cl_uint> expected_output;
            for( int i = 0; i < alloc_count; ++i )
            {
                expected_output.insert( expected_output.end(), pattern.cbegin(), pattern.cend() );
            }
            CHECK( results == expected_output );
        }
    }
}