#include "program_source_repository.h"
#include "utils.h"

namespace
{
    const char* kCommonSource = R"(
// Some OpenCL implementations don't implement that macro (yikes!)
#ifndef NULL
#   define NULL ((void*)0)
#endif
)";

    const char* openclMathSource = R"(
int Pow2ForInt(int x)
{
    return (x<0) ? 0 : (1<<x);
}
)";

    const char* kochCurveSource = R"(
typedef struct Line
{
    REAL_T_4 coords;
    // First number (x) is an iteration number, second is line identifier in current iteration
    int2 ids;
} Line;

// TODO for invalid values report error in some way or return 0?
int CalcLinesNumberForIteration(int iterationCount)
{
    return Pow2ForInt(2*iterationCount);
}

// TODO for invalid values report error in some way or return 0?
int CalcGlobalId(int2 ids)
{
    int result = ids.y;
    // TODO this function can be optimized by caching these values
    for (int iterationNumber = 0; iterationNumber < ids.x; ++iterationNumber)
    {
        result += CalcLinesNumberForIteration(iterationNumber);
    }
    return result;
}
)";

    const char* globalMemoryPoolSource = R"(
#define GLOBAL_MEMORY_POOL_MAX_SIZE UINT_MAX
/*
    Memory pool that uses global memory to store data.
    It needs a memory buffer allocated from host code, pointer to it should be
    pasted as a kernel parameter and then to InitGlobalMemoryPool as a memory_start parameter.
    Size of this buffer should be passed to the same function as a max_space_bytes parameter.
    Buffer should be initialized by zeroes.
    Maximum size of a buffer is limited by device capabilities (CL_DEVICE_MAX_MEM_ALLOC_SIZE) 
    or 2^32 bytes-1 (GLOBAL_MEMORY_POOL_MAX_SIZE),
    whichever is smaller.
    This implementation doesn't support deallocation.
*/
struct GlobalMemoryPool
{
    __global volatile uchar* memory_start;
    __global volatile uint* busy_space_counter;
    uint max_space_bytes;
};

// TODO avoid data erasure on multiple initializations
void InitGlobalMemoryPool(struct GlobalMemoryPool* pool, __global volatile uchar* memory_start, uint max_space_bytes)
{
    if (max_space_bytes <= sizeof(uint))
    {
        // TODO return error
        return;
    }
    // Store space counter in the memory buffer in the beginning
    pool->busy_space_counter = (__global volatile uint*)memory_start;
    pool->memory_start = memory_start + sizeof(uint);
    pool->max_space_bytes = max_space_bytes - sizeof(uint);
    if (max_space_bytes <= sizeof(uint))
    {
        // TODO return error
        return;
    }
}

__global volatile uchar* AllocateMemory(struct GlobalMemoryPool* pool, uint size)
{
    uint start = atomic_add(pool->busy_space_counter, size);

    // Here we calculate the sum again, but locally and we don't care about an actual value at this point
    if (start + size > pool->max_space_bytes)
    {
        // TODO return error
        return NULL;
    }
    if (start + size < start) // Check if we overflow
    {
        // TODO return error
        return NULL;
    }
    return pool->memory_start + start;
}
)";
}

std::string ProgramSourceRepository::GetOpenCLMathSource()
{
    return Utils::CombineStrings( { kCommonSource, openclMathSource } );
}

std::string ProgramSourceRepository::GetKochCurveSource()
{
    return Utils::CombineStrings( { kCommonSource,
        GetOpenCLMathSource(),
        GetGlobalMemoryPoolSource(),
        kochCurveSource
    } );
}

std::string ProgramSourceRepository::GetGlobalMemoryPoolSource()
{
    return Utils::CombineStrings( { kCommonSource, globalMemoryPoolSource } );
}
