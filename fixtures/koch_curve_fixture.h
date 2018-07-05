#pragma once

#include "fixtures/fixture.h"
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

		storage[currentId] = (Line){ .coords = coords, .ids = ids };
		newStart = newEnd;
	}
}

/*
    Process all lines on iteration level "startIteration",
    process all levels up to iteration level "stopAtIteration"
*/
void KochCurveCalc(int startIteration, int stopAtIteration, __global Line* linesTempStorage)
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

			__global Line* parentLine = linesTempStorage + parentLineId;
			ProcessLine(*parentLine, transformationMatrices, linesTempStorage);
		}
	}
}

ulong MinSpaceNeededInLineTempStorage(int stopAtIteration)
{
    return CalcLinesNumberForIteration(stopAtIteration) * sizeof(Line);
}

/*
	Build Koch's curve. This kernel iterates all levels up and including "stopAtIteration".
*/
__kernel void KochCurvePrecalculationKernel(int stopAtIteration,
    __global void* linesTempStorage, ulong linesTempStorageSize)
{
    if (get_global_size(0) > 1)
    {
        // TODO report error
        return;
    }
    if(linesTempStorageSize < MinSpaceNeededInLineTempStorage(stopAtIteration))
    {
        // TODO report error
        // TODO Don't think this check works
        return;
    }
    int startIteration = 0;
    __global Line* linesTempStorageConverted = (__global Line*)linesTempStorage;
    linesTempStorageConverted[0] = (Line){.coords = {0, 0, 1, 0}, .ids = {0, 0}};
    KochCurveCalc(startIteration, stopAtIteration, linesTempStorageConverted);;
}

REAL_T_4 KochCurveTransformLineToCurve(Line line, REAL_T_4 transformMatrix, REAL_T_2 offset)
{
    REAL_T_2 start = line.coords.lo;
    REAL_T_2 end = line.coords.hi;
    REAL_T_2 newStart = MultiplyMatrix2x2AndVector(transformMatrix, start) + offset;
    REAL_T_2 newEnd = MultiplyMatrix2x2AndVector(transformMatrix, end) + offset;
    return (REAL_T_4)(newStart, newEnd);
}

void KochSnowflakeCalc(int stopAtIteration, __global REAL_T_4* curves, int curvesCount, 
    __global Line* linesTempStorageConverted,
    __global REAL_T_4* out)
{
    size_t id = get_global_id(0);
    size_t resultStartIndex = id * curvesCount;
    size_t lineIndex = CalcGlobalId( (int2)(stopAtIteration, id) );
    for (int i = 0; i < curvesCount; ++i)
    {
        REAL_T_4 curve = curves[i];
        REAL_T_2 curveVector = curve.hi - curve.lo;
        REAL_T_2 curveVectorNorm = normalize(curveVector);
        REAL_T_4 transformMatrix = length(curveVector) *
            (REAL_T_4)(curveVectorNorm.x, -curveVectorNorm.y, curveVectorNorm.y, curveVectorNorm.x);
        REAL_T_2 offset = curve.lo;
        out[resultStartIndex + i] = KochCurveTransformLineToCurve(linesTempStorageConverted[lineIndex], transformMatrix, offset);
    }
}

/*
    Kernel allows to set where curves start and stop plus amount of curves.
    Every curve is just a general Koch curve, but combining them you can build
    different figures like Koch snowflakes.

    Lines are stored in "out" as following:
    <line 0 curve 0> <line 0 curve 1> <line 0 curve 2> <line 1 curve 0> <line 1 curve 1> <line 1 curve 2>...

    This kernel should be started with global size equal to number of lines.
    One work item equals to one line.

    TODO add option to leave lines from older iterations
*/
__kernel void KochSnowflakeKernel(int stopAtIteration,
    __global REAL_T_4* curves, int curvesCount,
    __global void* linesTempStorage, ulong linesTempStorageSize, 
    __global REAL_T_4* out )
{
    if(linesTempStorageSize < MinSpaceNeededInLineTempStorage(stopAtIteration))
    {
        // TODO report error
        return;
    }
    if(get_global_size(0) != CalcLinesNumberForIteration(stopAtIteration))
    {
        // TODO report error
        return;
    }
    __global Line* linesTempStorageConverted = (__global Line*)linesTempStorage;
    KochSnowflakeCalc(stopAtIteration, curves, curvesCount, linesTempStorageConverted, out);
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

    template<typename T>
    bool IsPointInViewport(T x, T y, long double width, long double height)
    {
        return x >= 0 && y >= 0 && x <= width && y <= height;
    }
}

/*
    T should be a floating point type (e.g. float, double or half),
    T2 should be a vector of 2 elements of the same type,
    T4 should be a vector of 4 elements of the same type,
*/
template<typename T, typename T2, typename T4>
class KochCurveFixture : public Fixture
{
public:
    explicit KochCurveFixture( 
        int iterationsCount,
        // Vector of lines. Every line becomes a curve that starts at (l.x; l.y) and ends at (l.z; l.w)
        const std::vector<T4>& curves,
        long double width, long double height,
        const std::string& descriptionSuffix = std::string() )
        : iterationsCount_(iterationsCount)
        , width_(width)
        , height_(height)
        , descriptionSuffix_(descriptionSuffix)
        , curves_(curves)
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

        std::string source = Utils::CombineStrings( {
            ProgramSourceRepository::GetKochCurveSource(),
            kochCurveProgramCode
        } );

        programs_.insert( {context.get(),
            Utils::BuildProgram( context, source, compilerOptions,
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
        std::vector<std::string> result = { "cl_khr_global_int32_base_atomics" };
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

        const size_t lineSizeInBytes = 64; // TODO find a better way to calculate this value to avoid wasting memory
        static_assert( lineSizeInBytes >= sizeof(T4) + 8, "Capacity allocated for one line is not sufficient" );
        const size_t linesTempStorageSizeInBytes = CalcTotalLineCount() * lineSizeInBytes;
        boost::compute::buffer linesTempStorage( context, linesTempStorageSizeInBytes );
        // Precalculation step
        {
            boost::compute::kernel kernel( programs_.at( context.get() ), "KochCurvePrecalculationKernel" );
            kernel.set_arg( 0, iterationsCount_ );

            kernel.set_arg( 1, linesTempStorage );
            kernel.set_arg( 2, linesTempStorageSizeInBytes );

            const unsigned minThreadsCount = 4;
            // TODO processingElementsCount should be a compute_units() multiplied with
            // a CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE
            unsigned processingElementsCount = context.get_device().compute_units();
            //unsigned threadsCount = std::max( minThreadsCount, processingElementsCount );
            unsigned threadsCount = 1u;
            events.insert( {OperationStep::Calculate,
                queue.enqueue_1d_range_kernel( kernel, 0, threadsCount, 0 )
            } );
        }

        // Final step
        // TODO include data copy in benchmark
        //boost::compute::vector<T4> curvesDeviceVector( curves_.cbegin(), curves_.cend(), queue );
        boost::compute::vector<T4> curvesDeviceVector( curves_.size(), context );
        boost::compute::copy( curves_.cbegin(), curves_.cend(), curvesDeviceVector.begin(), queue );
        // TODO avoid initialization on release build and use it on debug build
        size_t resultLineCount = CalcLineCount() * curves_.size();
        boost::compute::vector<T4> resultDeviceVector( resultLineCount, context );
        {
            boost::compute::kernel kernel( programs_.at( context.get() ), "KochSnowflakeKernel" );
            kernel.set_arg( 0, iterationsCount_ );

            kernel.set_arg( 1, curvesDeviceVector );
            kernel.set_arg( 2, static_cast<cl_int>( curvesDeviceVector.size() ) );

            kernel.set_arg( 3, linesTempStorage );
            kernel.set_arg( 4, static_cast<cl_ulong>( linesTempStorageSizeInBytes ) );

            kernel.set_arg( 5, resultDeviceVector );

            const unsigned threadsCount = CalcLineCount( iterationsCount_ );
            events.insert( {OperationStep::Calculate,
                queue.enqueue_1d_range_kernel( kernel, 0, threadsCount, 0 )
            } );
        }

        boost::compute::event event;
        void* outputDataPtr = queue.enqueue_map_buffer_async(
            resultDeviceVector.get_buffer(), CL_MAP_WRITE, 0,
            resultDeviceVector.size() * sizeof(T4),
            event );
        events.insert( {OperationStep::MapOutputData, event} );
        event.wait();

        const T4* outputDataPtrCasted = reinterpret_cast<const T4*>( outputDataPtr );
        const T4* endIterator = outputDataPtrCasted + resultDeviceVector.size();
        std::copy( outputDataPtrCasted, endIterator, std::back_inserter(outputData_) );

        boost::compute::event lastEvent = queue.enqueue_unmap_buffer( resultDeviceVector.get_buffer(), outputDataPtr );
        events.insert( {OperationStep::UnmapOutputData, lastEvent } );
        lastEvent.wait();

        return Utils::CalculateTotalStepDurations<Fixture::Duration>( events );
    }

    std::string Description() override
    {
        std::string result = ( boost::format( "Koch curve, %1%, %2% iterations (%3% lines)" ) %
            KochCurveFixtureConstants<T>::descriptionTypeName %
            iterationsCount_ %
            ( CalcLineCount() * curves_.size() )
            ).str();
        if( !descriptionSuffix_.empty() )
        {
            result += ", " + descriptionSuffix_;
        }
        return result;
    }

    virtual boost::optional<size_t> GetElementsCount() override
    {
        return CalcLineCount();
    }

    virtual void VerifyResults() override
    {
        // Verify that all points are within viewport (limited by width and height)
        if (!std::all_of(outputData_.cbegin(), outputData_.cend(),
            [&] (const T4& line) -> bool
            {
                return IsPointInViewport(line.x, line.y, width_, height_) &&
                    IsPointInViewport( line.z, line.w, width_, height_ );
            } ))
        {
        }
    }

    virtual void WriteResults() override
    {
        SVGDocument document;
        document.SetSize( width_, height_ );
        for( const auto& line : outputData_ )
        {
            document.AddLine(
                line.x,
                line.y,
                line.z,
                line.w
            );
        }
        document.BuildAndWriteToDisk( this->Description() + ".svg" );
    }

    virtual ~KochCurveFixture()
    {
    }
private:
    int iterationsCount_;
    std::vector<T4> outputData_;
    std::string descriptionSuffix_;
    std::unordered_map<cl_context, boost::compute::program> programs_;
    std::vector<T4> curves_;
    long double width_, height_;

    size_t CalcLineCount()
    {
        return CalcLineCount( iterationsCount_ );
    }

    size_t CalcLineCount( int i )
    {
        size_t a = 1;
        return a << ( 2 * i ); // Same as 4 ^ i
    }

    size_t CalcTotalLineCount( int iterCount )
    {
        size_t result = 0;
        for (int i = 0; i <= iterCount; ++i)
        {
            result += CalcLineCount( i );
        }
        return result;
    }

    size_t CalcTotalLineCount()
    {
        return CalcTotalLineCount( iterationsCount_ );
    }

    /*
    Return results in the following form:
    {
    { x1, y1, x2, y2 },
    ...
    }
    Every line is one line, where (x1, y1) is starting point and (x2, y2) is ending point
    */
    std::vector<std::vector<long double>> GetResults()
    {
        std::vector<std::vector<long double>> result;
        std::transform( outputData_.cbegin(), outputData_.cend(), std::back_inserter( result ),
            []( const T4& data )
        {
            return std::vector<long double>( {data.x, data.y, data.z, data.w} );
        } );
        return result;
    }
};