#pragma once

#include "fixture_that_returns_data.h"
#include "program_source_repository.h"
#include "koch_curve_utils.h"

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
*/

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

void ProcessLine(Line parentLine, __private REAL_T_4* transformationMatrices,
	__global Line* storage)
{
	REAL_T_2 start = parentLine.coords.lo;
	REAL_T_2 end = parentLine.coords.hi;
	REAL_T_2 vector = end - start;
	REAL_T_2 newStart = start;
	for (int i = 0; i < 4; ++i)
	{
		REAL_T_2 newEnd = MultiplyMatrix2x2AndVector(transformationMatrices[i], vector) + newStart;
		// New iteration number is one more than parent's one,
		// identifier is calculated using a formula based on a parent's identifier
		int2 ids = (int2)(parentLine.ids.x + 1, 4*parentLine.ids.y + i);
        size_t currentId = CalcGlobalId(ids);
        REAL_T_4 coords = MergePointsVectorsToLineVector(newStart, newEnd);

        //printf("Adding child line at index %d\n", currentId);
        //printf("Coords %v4hlf \n", coords);
        //printf("First value %d, second value %v4hlf \n", 100, coords);

		storage[currentId] = (Line){ .coords = coords, .ids = ids };
		newStart = newEnd;
	}
}

void KochCurveImplementation(int startIteration, int stopAtIteration, __global Line* inout)
{
	REAL_T_4 transformationMatrices[4];
	BuildTransformationMatrices(transformationMatrices);
	// Calculate total number of intermediate operations needed
	for (int iterationNumber = startIteration; iterationNumber < stopAtIteration; ++iterationNumber)
	{
		int lineCount = CalcLinesNumberForIteration(iterationNumber);
		for (int i = 0; i < lineCount; ++i)
		{
            size_t parentLineId = CalcGlobalId((int2)(iterationNumber, i));

            //printf("Processing parent line at index %d\n", parentLineId);

			__global Line* parentLine = inout + parentLineId;
			ProcessLine(*parentLine, transformationMatrices, inout);
		}
	}
}

/*
	Build Koch's curve. This kernel iterates on all lines on iteration level "startIteration"
	and iterate all levels up and including "stopAtIteration".
	Input data should be stored in "inout" buffer before starting this kernel.
	So if you want to start iteration ("startIteration" is zero), "inout" should contain
	exactly one value.
	The following convention is encouraged to simplify calculations:
		- start point is (0; 0), ends at (1; 0).
	
	Unsafe variant - few first iterations read and write data
	to the beginning of "output" without synchronization
	
	Output format is the following:
	Line.coords has coordinates for a line stored as following:
		startPoint.x                   startPoint.y
		endPoint.x(accessible via z)   endPoint.y(accessible via w)
	Line.ids contain identificators of a line stored in the following way:
		ids.x contain iteration number, ids.y contain line number
*/
__kernel void KochCurveKernel(int startIteration, int stopAtIteration, __global Line* inout )
{
/*
  if (get_global_size(0) > 1)
  {
      // TODO report error
      return;
  }
*/
  KochCurveImplementation(startIteration, stopAtIteration, inout);
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
        // TODO all warnings are disabled by -w, remove this option everywhere
        std::string typeName = KochCurveFixtureConstants<T>::openclTypeName;
        std::string compilerOptions = ( boost::format(
            "%1% -DREAL_T=%2% -DREAL_T_2=%3% -DREAL_T_4=%4%") %
            baseCompilerOptions % 
            typeName % 
            (typeName+"2") %
            ( typeName + "4" )
            ).str();

        std::string source = Utils::CombineStrings( 
            { ProgramSourceRepository::GetKochCurveSource(), kochCurveProgramCode } );

        kernels_.insert( {context.get(),
            Utils::BuildKernel( "KochCurveKernel", context, source, compilerOptions,
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
        typedef KochCurveUtils::Line<T4> Line;
        EXCEPTION_ASSERT( context.get_devices().size() == 1 );
        outputData_.clear();
        // create command queue with profiling enabled
        boost::compute::command_queue queue(
            context, context.get_device(), boost::compute::command_queue::enable_profiling
        );

        std::unordered_multimap<OperationStep, boost::compute::event> events;

        boost::compute::vector<KochCurveUtils::Line<T4>> inout_device_vector( 
            CalcTotalLineCount(), context );
        inout_device_vector.at(0) = KochCurveUtils::Line<T4>( {0, 0, 1, 0}, {0, 0} );

        boost::compute::kernel& kernel = kernels_.at( context.get() );

        const unsigned minThreadsCount = 4;
        // TODO processingElementsCount should be a compute_units() multiplied with
        // a CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE
        unsigned processingElementsCount = context.get_device().compute_units();
        //unsigned threadsCount = std::max( minThreadsCount, processingElementsCount );
        unsigned threadsCount = 1u;

        kernel.set_arg( 0, 0 );
        kernel.set_arg( 1, iterationsCount_ );
        kernel.set_arg( 2, inout_device_vector );

        events.insert( {OperationStep::Calculate,
            queue.enqueue_1d_range_kernel( kernel, 0, threadsCount, 0 )
        } );

        boost::compute::event event;
        void* outputDataPtr = queue.enqueue_map_buffer_async(
            inout_device_vector.get_buffer(), CL_MAP_WRITE, 0,
            inout_device_vector.size()*sizeof(Line),
            event );
        events.insert( {OperationStep::MapOutputData, event} );
        event.wait();

        const Line* outputDataPtrCasted = reinterpret_cast<const Line*>( outputDataPtr );
        const Line* endIterator = outputDataPtrCasted + inout_device_vector.size();
        std::transform( outputDataPtrCasted, endIterator,
            std::back_inserter(outputData_),
            [] (const Line& l) -> T4
        {
            return l.coords;
        } );

        boost::compute::event lastEvent = queue.enqueue_unmap_buffer( inout_device_vector.get_buffer(), outputDataPtr );
        events.insert( {OperationStep::UnmapOutputData, lastEvent } );
        lastEvent.wait();

        return Utils::CalculateTotalStepDurations<Fixture::Duration>( events );
    }

    std::string Description() override
    {
        std::string result = ( boost::format( "Koch curve, %1%, up to %2% iterations (%3% lines)" ) %
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

    size_t CalcTotalLineCount( int iterCount )
    {
        size_t result = 0;
        for (int i = 0; i <= iterCount; ++i)
        {
            result += GetLineCount( i );
        }
        return result;
    }

    size_t CalcTotalLineCount()
    {
        return CalcTotalLineCount( iterationsCount_ );
    }
};

//#include "damped_wave_fixture.inl"