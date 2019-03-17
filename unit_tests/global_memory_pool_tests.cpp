#include "catch/single_include/catch.hpp"

#include <vector>

#include "program_source_repository.h"
#include "utils.h"

namespace
{
    const char* memoryPoolSource = R"(
/*
    Initialiazes a memory pool and then returns offsets of all assigned memory areas.
    results should hold at least "allocCount" values
*/
__kernel void VerifyHappyPathMemoryAllocSinglethreaded(__global volatile uchar* mem, uint size, uint allocCount, uint allocSize, __global ulong* results)
{
    if (size < allocCount * allocSize + sizeof(uint))
    {
        // TODO report error
        return;
    }
    struct GlobalMemoryPool memoryPool;
    InitializeGlobalMemoryPool(&memoryPool, mem, size);
    __global volatile uchar* firstPtr = AllocateMemory(&memoryPool, allocSize);
    results[0] = 0;
    for (int i = 1; i < allocCount; ++i)
    {
        __global volatile uchar* ptr = AllocateMemory(&memoryPool, allocSize);
        results[i] = (ulong)(ptr - firstPtr);
    }
}

/*
    Initialiazes a memory pool and returns offsets of all assigned memory areas.
    results should hold at least "allocCount" values
*/
__kernel void VerifyHappyPathMemoryAllocMultithreaded(__global volatile uchar* mem, uint size, uint allocSize, __global ulong* results)
{
    struct GlobalMemoryPool memoryPool;
    InitializeGlobalMemoryPool(&memoryPool, mem, size);
    size_t id = get_global_id(0);
    results[id] = (ulong)AllocateMemory(&memoryPool, allocSize);
}

/*
    Initialiazes a memory pool, then every thread allocates space for 10 uint values and writes values from 1 to 10 to this space.
*/
__kernel void VerifyHappyPathWriteToAllocatedMemoryMultithreaded(__global volatile uchar* mem, uint size, uint allocSize)
{
    if (allocSize != 10 * sizeof(uint))
    {
        // TODO return error
        return;
    }
    struct GlobalMemoryPool memoryPool;
    InitializeGlobalMemoryPool(&memoryPool, mem, size);
    size_t id = get_global_id(0);
    __global volatile uint* data = AllocateMemory(&memoryPool, allocSize);
    for (uint i = 0; i < 10; ++i)
    {
        data[i] = i + 1;
    }
}
)";
}

TEST_CASE( "Happy path memory allocation from one thread", "[Global memory pool tests]" ) {
    const cl_uint allocSize = 10;
    const cl_uint allocCount = 10;
    const cl_uint totalMemorySizeInInts = 1000;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            boost::compute::vector<cl_int> buf( totalMemorySizeInInts, 0, queue ); // Memory pool
            boost::compute::vector<cl_ulong> outputDeviceVector( 10, context );

            boost::compute::kernel kernel = Utils::BuildKernel( "VerifyHappyPathMemoryAllocSinglethreaded",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetGlobalMemoryPoolSource(), memoryPoolSource} ),
                {},
                {"cl_khr_global_int32_base_atomics"} );
            kernel.set_arg( 0, buf );
            kernel.set_arg( 1, static_cast<cl_uint>( totalMemorySizeInInts * sizeof( cl_int ) ) );
            kernel.set_arg( 2, allocCount );
            kernel.set_arg( 3, allocSize );
            kernel.set_arg( 4, outputDeviceVector );
            queue.enqueue_task( kernel ).wait();

            // create vector on host
            std::vector<cl_ulong> results( allocCount );

            // copy data back to host
            boost::compute::copy(
                outputDeviceVector.begin(), outputDeviceVector.end(), results.begin(), queue
            );

            std::vector<cl_ulong> expectedOutput = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
            CHECK( results == expectedOutput );
        }
    }
}

TEST_CASE( "Happy path memory allocation from multiple threads", "[Global memory pool tests]" ) {
    const cl_uint allocSize = 10;
    const cl_uint allocCount = 10;
    const cl_uint totalMemorySizeInInts = 1000;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            boost::compute::vector<cl_int> buf( totalMemorySizeInInts, 0, queue ); // Memory pool
            boost::compute::vector<cl_ulong> outputDeviceVector( 10, context );

            boost::compute::kernel kernel = Utils::BuildKernel( "VerifyHappyPathMemoryAllocMultithreaded",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetGlobalMemoryPoolSource(), memoryPoolSource} ),
                {},
                {"cl_khr_global_int32_base_atomics"} );
            kernel.set_arg( 0, buf );
            kernel.set_arg( 1, static_cast<cl_uint>( totalMemorySizeInInts * sizeof( cl_int ) ) );
            kernel.set_arg( 2, allocSize );
            kernel.set_arg( 3, outputDeviceVector );
            queue.enqueue_1d_range_kernel( kernel, 0, allocCount, 0 ).wait();

            // create vector on host
            std::vector<cl_ulong> results( allocCount );

            // copy data back to host
            boost::compute::copy(
                outputDeviceVector.begin(), outputDeviceVector.end(), results.begin(), queue
            );

            // Kernel returned pointers stored as unsigned integers. First thing we should do is to sort them so we have them
            // in ascending order
            std::sort( results.begin(), results.end() );
            // Now remove the first value from all of them (first value is a base memory address, all others will be greater than it
            // and relative to it)
            cl_ulong firstAddress = results.at( 0 );
            std::transform( results.begin(), results.end(), results.begin(),
                [firstAddress]( cl_ulong val )
            {
                return val - firstAddress;
            } );

            std::vector<cl_ulong> expectedOutput = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
            CHECK( results == expectedOutput );
        }
    }
}

TEST_CASE( "Write data in multiple threads", "[Global memory pool tests]" ) {
    std::vector<cl_uint> pattern = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    const cl_uint allocSize = pattern.size() * sizeof( cl_uint );
    const cl_uint allocCount = 10;
    const cl_uint totalMemorySizeInInts = 1000;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        for( boost::compute::device& device : platform.devices() )
        {
            INFO( "Running test case for platform " << platform.name() << " and device " << device.name() );
            boost::compute::context context( device );
            boost::compute::command_queue queue( context, device );

            boost::compute::vector<cl_uint> buf( totalMemorySizeInInts, 0, queue ); // Memory pool

            boost::compute::kernel kernel = Utils::BuildKernel( "VerifyHappyPathWriteToAllocatedMemoryMultithreaded",
                context,
                Utils::CombineStrings( {ProgramSourceRepository::GetGlobalMemoryPoolSource(), memoryPoolSource} ),
                {},
                {"cl_khr_global_int32_base_atomics"} );
            kernel.set_arg( 0, buf );
            kernel.set_arg( 1, static_cast<cl_uint>( totalMemorySizeInInts * sizeof( cl_int ) ) );
            kernel.set_arg( 2, allocSize );
            queue.enqueue_1d_range_kernel( kernel, 0, allocCount, 0 ).wait();

            // create vector on host
            std::vector<cl_uint> results( allocCount * pattern.size() );

            // copy data back to host
            boost::compute::copy_n(
                buf.begin() + 1, allocCount * pattern.size(), results.begin(), queue
            );

            std::vector<cl_uint> expectedOutput;
            for( int i = 0; i < allocCount; ++i )
            {
                expectedOutput.insert( expectedOutput.end(), pattern.cbegin(), pattern.cend() );
            }
            CHECK( results == expectedOutput );
        }
    }
}