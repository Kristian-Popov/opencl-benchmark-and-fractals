#include "program_source_repository.h"
#include "utils.h"

namespace
{
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
    pasted as a kernel parameter and then to InitializeGlobalMemoryPool as a memoryStart parameter.
    Size of this buffer should be passed to the same function as a maxSpaceInBytes parameter.
    Buffer should be initialized by zeroes.
    Maximum size of a buffer is limited by device capabilities (CL_DEVICE_MAX_MEM_ALLOC_SIZE) 
    or 2^32 bytes-1 (GLOBAL_MEMORY_POOL_MAX_SIZE),
    whichever is smaller.
    This implementation doesn't support deallocation.
    
    Requires the following extensions enabled to work:
    - cl_khr_global_int32_base_atomics (for atom_add on uint)
*/
struct GlobalMemoryPool
{
    __global volatile uchar* memoryStart;
    __global volatile uint* busySpaceCounter;
    uint maxSpaceInBytes;
};

// TODO avoid data erasure on multiple initializations
void InitializeGlobalMemoryPool(struct GlobalMemoryPool* pool, __global volatile uchar* memoryStart, uint maxSpaceInBytes)
{
    if (maxSpaceInBytes <= sizeof(uint))
    {
        // TODO return error
        return;
    }
    // Store space counter in the memory buffer in the beginning
    pool->busySpaceCounter = (__global volatile uint*)memoryStart;
    pool->memoryStart = memoryStart + sizeof(uint);
    pool->maxSpaceInBytes = maxSpaceInBytes - sizeof(uint);
    if (maxSpaceInBytes <= sizeof(uint))
    {
        // TODO return error
        return;
    }
}

__global volatile uchar* AllocateMemory(struct GlobalMemoryPool* pool, uint size)
{
    uint start = atom_add(pool->busySpaceCounter, size);

    // Here we calculate the sum again, but locally and we don't care about an actual value at this point
    if (start + size > pool->maxSpaceInBytes)
    {
        // TODO return error
        return NULL;
    }
    if (start + size < start) // Check if we overflow
    {
        // TODO return error
        return NULL;
    }
    return pool->memoryStart + start;
}
)";
}

std::string ProgramSourceRepository::GetOpenCLMathSource()
{
    return openclMathSource;
}

std::string ProgramSourceRepository::GetKochCurveSource()
{
    return Utils::CombineStrings( { GetOpenCLMathSource(), GetGlobalMemoryPoolSource(), kochCurveSource } );
}

std::string ProgramSourceRepository::GetGlobalMemoryPoolSource()
{
    return globalMemoryPoolSource;
}
