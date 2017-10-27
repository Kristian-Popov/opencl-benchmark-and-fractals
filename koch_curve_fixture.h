#pragma once

#include "fixture_that_returns_data.h"
#include "boost/compute.hpp"

namespace
{
    // TODO due to this organization, all these variables classes collide
    // with other files, isolate them somehow
    static const char* kochCurveProgramCode = R"(
/*
Requires a definition:
- floating point type REAL_T (float, or, if device supports, half and double)
- REAL_T_2, REAL_T_4 (corresponding vector types with 2 and 4 elements respectively);
- STACK_SIZE is a maximum size of a stack;
- MAX_ITERATION_NUMBER is a maximum number of iterations
*/

/*
	Stack of lines
	Please keep in mind that it has no synchronization, so it's not safe to use
	from different threads.
	Data are allocated on private memory, so other processing elements don't have
	access to these data
*/
typedef struct Stack
{
	__private REAL_T_4 items[STACK_SIZE];
	int top;
} Stack;

void InitializeStack(__private Stack* stack)
{
    stack->top = 0;
}

void PushToStack(__private Stack* stack, REAL_T_4 value)
{
	// TODO does it hurt performance to check for over/underflows?
	if (STACK_SIZE == stack->top)
	{
	//TODO report error
	}
	else
	{
		stack->items[stack->top] = value;
		++(stack->top);
	}
}

REAL_T_4 PopFromStack(__private Stack* stack)
{
	// TODO does it hurt performance to check for over/underflows?
	if (0 == stack->top)
	{
	//TODO report error
	}
	else
	{
		--(stack->top);
		return stack->items[stack->top];
	}
}

int GetStackSize(__private Stack* stack)
{
	return stack->top;
}
//////////////////////////////////////////////////
/*
    Stores a list of line stacks, using an iteration number as a key
*/
typedef struct LineMap
{
    Stack stacks[MAX_ITERATION_NUMBER];
} LineMap;

void InitializeLineMap(__private LineMap* map)
{
    for (int i = 0; i < MAX_ITERATION_NUMBER; ++i)
    {
        InitializeStack(&(map->stacks[i]));
    }
}

__private Stack* GetStack(__private LineMap* map, int iterationNumber)
{
    //return &(map->stacks[iterationNumber]);
    return map->stacks + iterationNumber; 
}

////////////////////////////////////////////////////

REAL_T_4 MergePointsVectorsToLineVector(REAL_T_2 a, REAL_T_2 b)
{
	return (REAL_T_4)(a, b);
}

/*
	Multiply matrix 2x2 and 2-component vector.
	Also called Gemv in BLAS but limited to 2x2 matrices and 2-component vector
	
	Matrix should look like this:
	m.x   m.y
	m.z   m.w
*/
REAL_T_2 MultiplyMatrix2x2AndVector(REAL_T_4 m, REAL_T_2 v)
{
	return (REAL_T_2)(m.x*v.x + m.y*v.y, m.z*v.x + m.w*v.y);
}

void ProcessLineRecursively(__private LineMap* map, __private REAL_T_4* transformationMatrices, 
    int iterationIndex)
{
	REAL_T_4 t = PopFromStack(GetStack(map, iterationIndex));
	REAL_T_2 start = t.lo;
	REAL_T_2 end = t.hi;
	REAL_T_2 vector = end - start;
	REAL_T_2 newStart = start;
	for (int i = 0; i < 4; ++i)
	{
		REAL_T_2 newEnd = MultiplyMatrix2x2AndVector(transformationMatrices[i], vector) + newStart;
		PushToStack(GetStack(map, iterationIndex+1), MergePointsVectorsToLineVector(newStart, newEnd));
		newStart = newEnd;
	}
}

void ProcessAllRemainingLines(__private LineMap* map, __private REAL_T_4* transformationMatrices, int iterationIndex, 
    __global REAL_T_4* output)
{
  __private Stack* stack = GetStack(map, iterationIndex);
	int opNeeded = GetStackSize(stack);
	for (int operationIndex = 0; operationIndex < opNeeded; ++operationIndex)
	{
		REAL_T_4 t = PopFromStack(stack);
		REAL_T_2 start = t.lo;
		REAL_T_2 end = t.hi;
		REAL_T_2 vector = end - start;
		REAL_T_2 newStart = start;
		for (int i = 0; i < 4; ++i)
		{
			REAL_T_2 newEnd = MultiplyMatrix2x2AndVector(transformationMatrices[i], vector) + newStart;
			output[operationIndex*4 + i] = MergePointsVectorsToLineVector(newStart, newEnd);
			newStart = newEnd;
		}
	}
}

/*
	Calculates power of 2 of an integer argument.
	Returns 0 for arguments that are smaller than 0
*/
int Pow2ForInt(int x)
{
	return (x<0) ? 0 : (1<<x);
}

/*
	Fill an array with 4 transformation matrices.
*/
void BuildTransformationMatrices(__private REAL_T_4* transformationMatrices)
{
	transformationMatrices[0] = (REAL_T_4)(1.0/3.0, 0, 0, 1.0/3.0);
	transformationMatrices[1] = (REAL_T_4)(1.0/6.0, -1.0/(2*sqrt(3.0)), 1.0/(2*sqrt(3.0)), 1.0/6.0);
	transformationMatrices[2] = (REAL_T_4)(1.0/6.0, 1.0/(2*sqrt(3.0)), -1.0/(2*sqrt(3.0)), 1.0/6.0);
	transformationMatrices[3] = (REAL_T_4)(1.0/3.0, 0, 0, 1.0/3.0);
}

int CalcLinesNumberForIteration(int iterationCount)
{
    return Pow2ForInt(2*iterationCount);
}

void BuildKochCurve(int iterationsCount, __global REAL_T_4* output)
{
	LineMap map;
  InitializeLineMap(&map);
	PushToStack( GetStack(&map, 0), (REAL_T_4)(0, 0, 1, 0) );
	
	REAL_T_4 transformationMatrices[4];
	BuildTransformationMatrices(transformationMatrices);
	// Calculate total number of intermediate operations needed
	// using (interationsCount-1)
	// All data from intermediate operations will go to stack,
	// but data from finishing operations will go directly to output buffer
	int iterationNumber = 0;
  for (; iterationNumber < iterationsCount-1; ++iterationNumber)
  {
      int lineCount = CalcLinesNumberForIteration(iterationNumber);
      for (int i = 0; i < lineCount; ++i)
      {
          ProcessLineRecursively(&map, transformationMatrices, iterationNumber);
      }
  }
	ProcessAllRemainingLines(&map, transformationMatrices, iterationNumber, output);
}

/*
	Builds a Koch's curve. Start point is (0; 0), ends at (1; 0).
	Output format is the following:
	startPoint.x                   startPoint.y
	endPoint.x(accessible via z)   endPoint.y(accessible via w)
*/
__kernel void KochCurve(int iterationsCount, __global REAL_T_4* output )
{
  if (iterationsCount > MAX_ITERATION_NUMBER || iterationsCount < 1)
  {
      // TODO report error
      return;
  }
  if (STACK_SIZE < CalcLinesNumberForIteration(MAX_ITERATION_NUMBER))
  {
      // TODO report error
      return;
  }
	BuildKochCurve(iterationsCount, output);
}
)";

    template<typename T>
    struct KochCurveFixtureConstants
    {
        static const char* openclTypeName;
        static const char* requiredExtension;
        static const char* descriptionTypeName;
        static const int maxIterations;
    };

    const char* KochCurveFixtureConstants<cl_float>::openclTypeName = "float";
    const char* KochCurveFixtureConstants<cl_float>::requiredExtension = ""; // No extensions needed for single precision arithmetic
    const char* KochCurveFixtureConstants<cl_float>::descriptionTypeName = "float";
    const int KochCurveFixtureConstants<cl_float>::maxIterations = 20; //TODO update value

    const char* KochCurveFixtureConstants<cl_double>::openclTypeName = "double";
    const char* KochCurveFixtureConstants<cl_double>::requiredExtension = "cl_khr_fp64";
    const char* KochCurveFixtureConstants<cl_double>::descriptionTypeName = "double";
    const int KochCurveFixtureConstants<cl_double>::maxIterations = 20;

    // TODO support for half precision could be added by creating an alternate data types,
    // something like half2 and half4
}

/*
    T should be a floating point type (e.g. float, double or half),
    T2 should be a vector of 2 elements of the same type,
    T4 should be a vector of 4 elements of the same type,
*/
template<typename T, typename T2, typename T4>
/* FixtureThatReturnsData is used to retrieve data. If we have something like
FixtureThatReturnsData<T>, then we may have trouble writing multiple values into CSV/SVG/etc.
Instead we are using long double that is well suiting to store floating point values
since it can hold any half/float/double value without any precision loss
*/
class KochCurveFixture : public FixtureThatReturnsData<long double>
{
public:
    explicit KochCurveFixture( 
        int iterationsCount,
        const std::string& descriptionSuffix = std::string() )
        : iterationsCount_(iterationsCount)
        , descriptionSuffix_(descriptionSuffix)
    {
        static_assert( sizeof( T2 ) == 2 * sizeof( T ), "Given wrong second template argument to KochCurveFixture" );
        static_assert( sizeof( T4 ) == 4 * sizeof( T ), "Given wrong third template argument to KochCurveFixture" );
        EXCEPTION_ASSERT( iterationsCount >= 1 && iterationsCount <= KochCurveFixtureConstants<T>::maxIterations );
    }

    //virtual void Initialize() override;

    virtual void InitializeForContext( boost::compute::context& context ) override
    {
        std::string typeName = KochCurveFixtureConstants<T>::openclTypeName;
        std::string compilerOptions = ( boost::format(
            "%1% -DREAL_T=%2% -DREAL_T_2=%3% -DREAL_T_4=%4% -DSTACK_SIZE=%5% -DMAX_ITERATION_NUMBER=%6%") %
            baseCompilerOptions % 
            typeName % 
            (typeName+"2") %
            ( typeName + "4" ) %
            GetLineCount( iterationsCount_ ) %
            iterationsCount_
            ).str();

        kernels_.insert( {context.get(),
            Utils::BuildKernel( "KochCurve", context, kochCurveProgramCode, compilerOptions,
                GetRequiredExtensions() )} );
    }

    virtual std::vector<OperationStep> GetSteps() override
    {
        return {
            OperationStep::Calculate,
            OperationStep::MapOutputData,
            OperationStep::UnmapOutputData
        };
    }

    virtual std::vector<std::string> GetRequiredExtensions() override
    {
        std::string requiredExtension = KochCurveFixtureConstants<T>::requiredExtension;
        std::vector<std::string> result;
        if( !requiredExtension.empty() )
        {
            result.push_back( requiredExtension );
        }
        return result;
    }

    std::unordered_map<OperationStep, Duration> Execute( boost::compute::context& context ) override
    {
        EXCEPTION_ASSERT( context.get_devices().size() == 1 );
        outputData_.clear();
        // create command queue with profiling enabled
        boost::compute::command_queue queue(
            context, context.get_device(), boost::compute::command_queue::enable_profiling
        );

        std::unordered_multimap<OperationStep, boost::compute::event> events;

        boost::compute::vector<T4> output_device_vector( GetLineCount(), context );

        boost::compute::kernel& kernel = kernels_.at( context.get() );

        kernel.set_arg( 0, iterationsCount_ );
        kernel.set_arg( 1, output_device_vector.get_buffer() );

        unsigned computeUnitsCount = context.get_device().compute_units();
        size_t localWorkGroupSize = 0;
        events.insert( {OperationStep::Calculate,
            queue.enqueue_1d_range_kernel( kernel, 0, 1, localWorkGroupSize )
        } );

        boost::compute::event event;
        void* outputDataPtr = queue.enqueue_map_buffer_async(
            output_device_vector.get_buffer(), CL_MAP_WRITE, 0,
            output_device_vector.size()*sizeof(T4),
            event );
        events.insert( {OperationStep::MapOutputData, event} );
        event.wait();

        const T4* outputDataPtrCasted = reinterpret_cast<const T4*>( outputDataPtr );
        std::copy_n( outputDataPtrCasted, GetLineCount(), std::back_inserter(outputData_) );

        boost::compute::event lastEvent = queue.enqueue_unmap_buffer( output_device_vector.get_buffer(), outputDataPtr );
        events.insert( {OperationStep::UnmapOutputData, lastEvent } );
        lastEvent.wait();

        return Utils::CalculateTotalStepDurations<Fixture::Duration>( events );
    }

    std::string Description() override
    {
        std::string result = ( boost::format( "Koch curve, %1%, up to %2% iterations (%3% lines to total)" ) %
            KochCurveFixtureConstants<T>::descriptionTypeName %
            iterationsCount_ %
            GetLineCount()
            ).str();
        if( !descriptionSuffix_.empty() )
        {
            result += ", " + descriptionSuffix_;
        }
        return result;
    }

    void VerifyResults() {}

    /*
    Return results in the following form:
    {
    { x1, y1, x2, y2 },
    ...
    }
    Every line is one line, where (x1, y1) is starting point and (x2, y2) is ending point
    */
    virtual std::vector<std::vector<long double>> GetResults() override
    {
        std::vector<std::vector<long double>> result;
        std::transform( outputData_.cbegin(), outputData_.cend(), std::back_inserter(result),
            [] (const T4& data)
        {
            return std::vector<long double>( { data.x, data.y, data.z, data.w } );
        } );
        return result;
    }

    virtual boost::optional<size_t> GetElementsCount() override
    {
        return GetLineCount();
    }

    virtual ~KochCurveFixture()
    {
    }
private:
    int iterationsCount_;
    std::vector<T4> outputData_;
    std::string descriptionSuffix_;
    std::unordered_map<cl_context, boost::compute::kernel> kernels_;

    size_t GetLineCount()
    {
        return GetLineCount( iterationsCount_ );
    }

    size_t GetLineCount( int i )
    {
        size_t a = 1;
        return a << ( 2 * i ); // Same as 4 ^ i
    }
};

//#include "damped_wave_fixture.inl"