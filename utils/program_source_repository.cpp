#include "program_source_repository.h"
#include "utils.h"

namespace
{
#if 0
    const char* untestedSource = R"(
/*
    Math functions to operate on multiprecision unsigned numbers.
    Currently only addition and multiplication is supported.
    Only unsigned numbers are supported at a moment.
    Words are stored in little endian format (word with the lowest address (i.e. first) is least significant),
    bytes in a word are stored using usual device endianness.

    Normalized values in case of 64-bit numbers means they must fit uint (take only first 32 bits).
*/

enum MultiprecisionMathErrors
{
    MULTIPRECISION_MATH_ERROR_OVERFLOW,
    MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED,
    UNKNOWN_ERROR
};

#define MAX_VAL CL_UINT_MAX

/*
    Normalize multiprecision value.
    lhs - left number (defined as array of words), lhsSize - number of words in lhs,

    This function is executed solely by one thread.

    Output value is normalized after this operation.
*/
__kernel void MultiprecisionNormalize( __global ulong* lhs, int lhsSize )
{
    uint remainder = 0;
    for( int i = 0; i < lhsSize; ++i )
    {
        // Max value in sum is 2^64 - 2^32
        ulong sum = lhs[i] + remainder;

        // Convert 64-bit number to a vector of 2 32-bit numbers
        // TODO is it portable if device and host use different endianess?
        uint2 data = vload2( 0, (__private uint*)(sum) );
        lhs[i] = data.lo;
        remainder = data.hi;
    }
}

/*
    Add multiprecision value with a single word value.
    lhs - left number (defined as array of words), lhsSize - number of words in lhs,
    rhs is a right number (a single word),
    output - output number, outputSize - number of words in output,

    This function is executed solely by one thread.

    Input value MUST be normalized.
    Output value is normalized after this operation.

    Currently works on 32-bit words only (but arrays are 64-bit to handle overflow gracefully)

    Requires:
        - cl_khr_global_int32_extended_atomics extension for error handling

    TODO:
        - parametrize word type (requires automatic detemining of masks and other values)
*/
__kernel void MultiprecisionAddWithSingleWord(
    __global ulong* lhs, int lhsSize,
    ulong rhs,
    __global ulong* output, int outputSize,
    __global uint* errors
)
{
    if( outputSize < ( lhsSize + 1 ) )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_OVERFLOW );
        return;
    }
    if( rhs > MAX_VAL )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED );
        return;
    }

    for( int i = 0; i < lhsSize; ++i )
    {
        if( lhs[i] > MAX_VAL )
        {
            atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED );
            return;
        }

        output[i] = lhs[i] + rhs;
    }

    MultiprecisionNormalize( output, outputSize );
}

/*
    Multiply multiprecision value with a single word.
    lhs - left number (defined as array of words), lhsSize - number of words in lhs,
    rhs is a right number (a single word),
    output - output number, outputSize - number of words in output,

    This function is executed solely by one thread.

    Input value MUST be normalized.
    Output value is normalized after this operation.

    Currently works on 32-bit words only (but arrays are 64-bit to handle overflow gracefully)

    Requires:
        - cl_khr_global_int32_extended_atomics extension for error handling

    TODO:
        - parametrize word type (requires automatic detemining of masks and other values)
*/
__kernel void MultiprecisionMultWithSingleWord(
    __global ulong* lhs, int lhsSize,
    ulong rhs,
    __global ulong* output, int outputSize,
    __global uint* errors
)
{
    if( outputSize < ( lhsSize + 1 ) )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_OVERFLOW );
        return;
    }
    if( rhs > MAX_VAL )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED );
        return;
    }

    for( int i = 0; i < lhsSize; ++i )
    {
        if( lhs[i] > MAX_VAL )
        {
            atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED );
            return;
        }
        /*
            We checked that lhs[i] and rhs are within lower 32 bits so
            max value we can get in output is ( 2^64 - 2^33 + 1 ),
            we're guaranteed to not have an overflow
        */
        output[i] = lhs[i] * rhs;
    }

    MultiprecisionNormalize( output, outputSize );
}

/*
    Multiply two multiprecision numbers

    lhs - left number (defined as array of words), lhsSize - number of words in lhs,
    rhs - right number, rhsSize - number of words in rhs,
    output - output number, outputSize - number of words in output,

    Every work item ("thread") takes a single word from "rhs", multiplies it with all words from "lhs"
    and puts all multiplication results to "output"
    Kernel can be called few times by passing appropriate IDs
    to split multiplication of very large numbers.
    Output is a matrix of a multiplied words.
    To finish multiplication, call MultiprecisionMultPhase2 function.

    Input value MUST be normalized.
    Output value is normalized after this operation.

    Currently works on 32-bit words only (but arrays are 64-bit to handle overflow gracefully)

    Requires:
        - cl_khr_global_int32_extended_atomics extension for error handling
*/
__kernel void MultiprecisionMultPhase1(
    __global ulong* lhs, int lhsSize,
    __global ulong* rhs, int rhsSize,
    __global ulong* temp, int tempSize,
    __global uint* errors
)
{
    size_t id = get_global_id( 0 );
    if( get_global_size( 0 ) > rhsSize || id > rhsSize )
    {
        atom_or( errors, 1u << UNKNOWN_ERROR ); // TODO imagine better name for that
        return;
    }
    ulong rhsWord = rhs[id];

    int tempRowSize = lhsSize + 1;
    if( tempSize < tempRowSize * rhsSize )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_OVERFLOW );
        return;
    }
    __global ulong* currentOutputPtr = temp + id * tempRowSize + id;
    MultiprecisionMultWithSingleWord( lhs, lhsSize, rhsWord, currentOutputPtr, tempRowSize, errors );
}

/*
    TODO description

    This kernel sums multiplied words.
    Every work item (thread) takes one column and adds all words in it.
    ID must be lower or equal to row size.

    output may be same as lhs or rhs.
*/
__kernel void MultiprecisionMultPhase2(
    __global ulong* lhs, int lhsSize,
    __global ulong* rhs, int rhsSize,
    __global ulong* temp, int tempSize,
    __global ulong* output, int outputSize,
    __global uint* errors
)
{
    int tempRowSize = lhsSize + 1;
    if( get_global_size( 0 ) > tempRowSize )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_OVERFLOW );
        return;
    }
    if( outputSize < lhsSize + rhsSize )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_OVERFLOW );
        return;
    }
    ulong sum = 0;
    for( int i = 0; i < rhsSize; ++i )
    {
        // TODO any conditions for overflow here?
        sum += temp + i * tempRowSize + id;
    }
    output[id] = sum;
}
)";
#endif

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

    const char* multiprecisionMathSource = R"(
/*
    Math functions to operate on multiprecision unsigned numbers.
    Currently only addition and multiplication is supported.
    Only unsigned numbers are supported at a moment.
    Words are stored in little endian format (word with the lowest address (i.e. first) is least significant),
    bytes in a word are stored using usual device endianness.

    Normalized values in case of 64-bit numbers means they must fit uint (take only first 32 bits).
*/

enum MultiprecisionMathErrors
{
    MULTIPRECISION_MATH_ERROR_OVERFLOW,
    MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED,
    UNKNOWN_ERROR
};

#define MAX_VAL UINT_MAX

/*
    Normalize multiprecision value.
    lhs - left number (defined as array of words), lhsSize - number of words in lhs,

    Must be run by a single thread.

    Normalization stops when remainder is equals to zero, so overflow should occur on
    less significant words only.

    Output value is normalized after this operation.
*/
__kernel void MultiprecisionNormalize(
    __global ulong* lhs, int lhsSize,
    __global uint* errors
)
{
    ulong remainder = 0;
    for( int i = 0; i < lhsSize; ++i )
    {
        // Max value in sum is 2^64 - 2^32
        ulong sum = lhs[i] + remainder;

        // Split 64-bit number to 2 32-bit numbers
        lhs[i] = sum & 0xFFFFFFFF;
        remainder = ( sum & 0xFFFFFFFF00000000 ) >> 32;
    }

    if( remainder > 0 )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_OVERFLOW );
        return;
    }
}

/*
    Multiply multiprecision value with a single word.
    lhs - left number (defined as array of words), lhsSize - number of words in lhs,
    rhs is a right number (a single word),
    output - output number, outputSize - number of words in output,

    Input value MUST be normalized.
    Output value is normalized after this operation.

    Currently works on 32-bit words only (but arrays are 64-bit to handle overflow gracefully)

    Requires:
        - cl_khr_global_int32_extended_atomics extension for error handling

    TODO:
        - parametrize word type (requires automatic detemining of masks and other values)
*/
__kernel void MultiprecisionMultWithSingleWord(
    __global ulong* lhs, int lhsSize,
    uint rhs,
    __global ulong* output, int outputSize,
    __global uint* errors
)
{
    size_t id = get_global_id( 0 );
    if( lhs[id] > MAX_VAL )
    {
        atom_or( errors, 1u << MULTIPRECISION_MATH_ERROR_NOT_NORMALIZED );
        return;
    }
    /*
        We checked that lhs[id] and rhs are within lower 32 bits so
        max value we can get in output is ( 2^64 - 2^33 + 1 ),
        we're guaranteed to not have an overflow
    */
    output[id] = lhs[id] * rhs;
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

std::string ProgramSourceRepository::GetMultiprecisionMathSource()
{
    return Utils::CombineStrings( { kCommonSource, multiprecisionMathSource } );
}
