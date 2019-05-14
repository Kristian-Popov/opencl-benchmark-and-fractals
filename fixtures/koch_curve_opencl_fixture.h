#pragma once

#include "fixtures/fixture.h"
#include "opencl_type_traits.h"
#include "program_source_repository.h"

#include "boost/compute.hpp"

namespace
{
    // TODO due to this organization, all these variables classes collide
    // with other files, isolate them somehow
    static const char* kKochCurveProgramCode = R"(
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
void BuildTransformationMatrices(__private REAL_T_4* transform_matrices)
{
    transform_matrices[0] = (REAL_T_4)(1.0/3.0, 0, 0, 1.0/3.0);
    transform_matrices[1] = (REAL_T_4)(1.0/6.0, -1.0/(2*sqrt(3.0)), 1.0/(2*sqrt(3.0)), 1.0/6.0);
    transform_matrices[2] = (REAL_T_4)(1.0/6.0, 1.0/(2*sqrt(3.0)), -1.0/(2*sqrt(3.0)), 1.0/6.0);
    transform_matrices[3] = (REAL_T_4)(1.0/3.0, 0, 0, 1.0/3.0);
}

void ProcessLine(Line parent_line, __private REAL_T_4* transform_matrices,
    __global Line* storage)
{
    REAL_T_2 start = parent_line.coords.lo;
    REAL_T_2 end = parent_line.coords.hi;
    REAL_T_2 vector = end - start;
    REAL_T_2 new_start = start;
    for (int i = 0; i < 4; ++i)
    {
        REAL_T_2 new_end = MultiplyMatrix2x2AndVector(transform_matrices[i], vector) + new_start;
        // New iteration number is one more than parent's one,
        // identifier is calculated using a formula based on a parent's identifier
        int2 ids = (int2)(parent_line.ids.x + 1, 4*parent_line.ids.y + i);
        size_t current_id = CalcGlobalId(ids);
        REAL_T_4 coords = MergePointsVectorsToLineVector(new_start, new_end);

        storage[current_id] = (Line){ .coords = coords, .ids = ids };
        new_start = new_end;
    }
}

/*
    Process all lines on iteration level "start_iteration", up to iteration level "stop_at_iteration"
*/
void KochCurveCalc(int start_iteration, int stop_at_iteration, __global Line* lines_temp_storage)
{
    REAL_T_4 transform_matrices[4];
    BuildTransformationMatrices(transform_matrices);
    // Calculate total number of intermediate operations needed
    for (int iter_number = start_iteration; iter_number < stop_at_iteration; ++iter_number)
    {
        int line_count = CalcLinesNumberForIteration(iter_number);
        for (int i = 0; i < line_count; ++i)
        {
            size_t parent_line_id = CalcGlobalId((int2)(iter_number, i));

            __global Line* parent_line = lines_temp_storage + parent_line_id;
            ProcessLine(*parent_line, transform_matrices, lines_temp_storage);
        }
    }
}

ulong MinSpaceNeeded(int stop_at_iteration)
{
    return CalcLinesNumberForIteration(stop_at_iteration) * sizeof(Line);
}

/*
    Build Koch's curve. This kernel iterates all levels up and including "stop_at_iteration".
*/
__kernel void KochCurvePrecalculationKernel(int stop_at_iteration,
    __global void* lines_temp_storage, ulong lines_temp_storage_size)
{
    if (get_global_size(0) > 1)
    {
        // TODO report error
        return;
    }
    if(lines_temp_storage_size < MinSpaceNeeded(stop_at_iteration))
    {
        // TODO report error
        // TODO Don't think this check works
        return;
    }
    int start_iteration = 0;
    __global Line* lines_temp_storage_conv = (__global Line*)lines_temp_storage;
    lines_temp_storage_conv[0] = (Line){.coords = {0, 0, 1, 0}, .ids = {0, 0}};
    KochCurveCalc(start_iteration, stop_at_iteration, lines_temp_storage_conv);
}

REAL_T_4 KochCurveTransformLineToCurve(Line line, REAL_T_4 transform_matrix, REAL_T_2 offset)
{
    REAL_T_2 start = line.coords.lo;
    REAL_T_2 end = line.coords.hi;
    REAL_T_2 new_start = MultiplyMatrix2x2AndVector(transform_matrix, start) + offset;
    REAL_T_2 new_end = MultiplyMatrix2x2AndVector(transform_matrix, end) + offset;
    return (REAL_T_4)(new_start, new_end);
}

void KochSnowflakeCalc(int stop_at_iteration, __global REAL_T_4* curves, int curves_count,
    __global Line* lines_temp_storage,
    __global REAL_T_4* out)
{
    size_t id = get_global_id(0);
    size_t result_start_index = id * curves_count;
    size_t line_index = CalcGlobalId( (int2)(stop_at_iteration, id) );
    for (int i = 0; i < curves_count; ++i)
    {
        REAL_T_4 curve = curves[i];
        REAL_T_2 curve_vector = curve.hi - curve.lo;
        REAL_T_2 curve_vector_norm = normalize(curve_vector);
        REAL_T_4 transform_matrix = length(curve_vector) *
            (REAL_T_4)(curve_vector_norm.x, -curve_vector_norm.y, curve_vector_norm.y, curve_vector_norm.x);
        REAL_T_2 offset = curve.lo;
        out[result_start_index + i] = KochCurveTransformLineToCurve(
            lines_temp_storage[line_index], transform_matrix, offset
        );
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
__kernel void KochSnowflakeKernel(int stop_at_iteration,
    __global REAL_T_4* curves, int curves_count,
    __global void* lines_temp_storage, ulong lines_temp_storage_size,
    __global REAL_T_4* out )
{
    if(lines_temp_storage_size < MinSpaceNeeded(stop_at_iteration))
    {
        // TODO report error
        return;
    }
    if(get_global_size(0) != CalcLinesNumberForIteration(stop_at_iteration))
    {
        // TODO report error
        return;
    }
    __global Line* lines_temp_storage_conv = (__global Line*)lines_temp_storage;
    KochSnowflakeCalc(stop_at_iteration, curves, curves_count, lines_temp_storage_conv, out);
}
)";

    // TODO support for half precision could be added by creating an alternate data type,
    // something like half2 and half4
}

/*
    T should be a floating point type (e.g. float, double or half),
    T4 should be a vector of 4 elements of the same type,
*/
template<typename T, typename T4>
class KochCurveOpenClFixture : public Fixture
{
public:
    explicit KochCurveOpenClFixture(
        const std::shared_ptr<OpenClDevice>& device,
        int iterations_count,
        // Vector of lines. Every line becomes a curve that starts at (l.x; l.y) and ends at (l.z; l.w)
        const std::vector<T4>& curves,
        double width, double height,
        const std::string& fixture_name
    )
        : device_( device )
        , iterations_count_(iterations_count)
        , width_(width)
        , height_(height)
        , curves_(curves)
        , fixture_name_( fixture_name )
    {
        static_assert( sizeof( T4 ) == 4 * sizeof( T ), "Given wrong second template argument to KochCurveOpenClFixture" );
        EXCEPTION_ASSERT( iterations_count >= 1 && iterations_count <= max_iterations_ );
    }

    virtual void Initialize() override
    {
        // TODO all warnings are disabled by -w, remove this option everywhere
        std::string type_name = OpenClTypeTraits<T>::type_name;
        std::string compiler_options = ( boost::format(
            "-DREAL_T=%1% -DREAL_T_2=%2% -DREAL_T_4=%3%" ) %
            type_name %
            ( type_name + "2" ) % // TODO replace this with clever preprocessor macros
            ( type_name + "4" )
            ).str();
        std::string source = Utils::CombineStrings( {
            ProgramSourceRepository::GetKochCurveSource(),
            kKochCurveProgramCode
        } );

        program_ = Utils::BuildProgram( device_->GetContext(), source, compiler_options,
            GetRequiredExtensions() );
    }

    virtual std::vector<std::string> GetRequiredExtensions() override
    {
        return CollectExtensions<T>();
    }

    std::unordered_map<OperationStep, Duration> Execute() override
    {
        boost::compute::context& context = device_->GetContext();
        boost::compute::command_queue& queue = device_->GetQueue();
        output_data_.clear();

        std::unordered_map<OperationStep, boost::compute::event> events;

        const size_t line_size_in_bytes = 64; // TODO find a better way to calculate this value to avoid wasting memory
        static_assert( line_size_in_bytes >= sizeof(T4) + 8, "Capacity allocated for one line is not sufficient" );
        const size_t line_temp_storage_size_in_bytes = CalcTotalLineCount() * line_size_in_bytes;
        boost::compute::buffer lines_temp_storage( context, line_temp_storage_size_in_bytes );
        // Precalculation step
        {
            boost::compute::kernel kernel( program_, "KochCurvePrecalculationKernel" );
            kernel.set_arg( 0, iterations_count_ );

            kernel.set_arg( 1, lines_temp_storage );
            kernel.set_arg( 2, line_temp_storage_size_in_bytes );

            // TODO implement parallelization
            events.insert( {OperationStep::Calculate1,
                queue.enqueue_1d_range_kernel( kernel, 0, 1, 0 )
            } );
        }

        // Final step
        // TODO include data copy in benchmark
        //boost::compute::vector<T4> curves_device_vector( curves_.cbegin(), curves_.cend(), queue );
        boost::compute::vector<T4> curves_device_vector( curves_.size(), context );
        boost::compute::copy( curves_.cbegin(), curves_.cend(), curves_device_vector.begin(), queue );
        // TODO avoid initialization on release build and use it on debug build
        size_t result_line_count = CalcLineCount() * curves_.size();
        boost::compute::vector<T4> result_device_vector( result_line_count, context );
        {
            boost::compute::kernel kernel( program_, "KochSnowflakeKernel" );
            kernel.set_arg( 0, iterations_count_ );

            kernel.set_arg( 1, curves_device_vector );
            kernel.set_arg( 2, static_cast<cl_int>( curves_device_vector.size() ) );

            kernel.set_arg( 3, lines_temp_storage );
            kernel.set_arg( 4, static_cast<cl_ulong>( line_temp_storage_size_in_bytes ) );

            kernel.set_arg( 5, result_device_vector );

            const unsigned threadsCount = CalcLineCount( iterations_count_ );
            events.insert( {OperationStep::Calculate2,
                queue.enqueue_1d_range_kernel( kernel, 0, threadsCount, 0 )
            } );
        }

        // TODO replace with MappedOpenClBuffer
        boost::compute::event event;
        void* output_data_ptr = queue.enqueue_map_buffer_async(
            result_device_vector.get_buffer(), CL_MAP_WRITE, 0,
            result_device_vector.size() * sizeof(T4),
            event );
        events.insert( {OperationStep::MapOutputData, event} );
        event.wait();

        const T4* output_data_ptr_casted = reinterpret_cast<const T4*>( output_data_ptr );
        const T4* end_iterator = output_data_ptr_casted + result_device_vector.size();
        std::copy( output_data_ptr_casted, end_iterator, std::back_inserter(output_data_) );

        boost::compute::event last_event = queue.enqueue_unmap_buffer( result_device_vector.get_buffer(), output_data_ptr );
        events.insert( {OperationStep::UnmapOutputData, last_event } );
        last_event.wait();

        return Utils::GetOpenCLEventDurations( events );
    }

    virtual void VerifyResults() override
    {
        // Verify that all points are within viewport (limited by width and height)
        auto wrong_point = std::find_if( output_data_.cbegin(), output_data_.cend(),
            [&]( const T4& line ) -> bool
        {
            return !( IsPointInViewport( line.x, line.y ) && IsPointInViewport( line.z, line.w ) );
        } );
        if( wrong_point != output_data_.cend() )
        {
            throw DataVerificationFailedException(
                ( boost::format( "Result verification has failed for Koch curve fixture. "
                    "At least one line is outside of viewpoint: %1%." ) %
                    LineToString( *wrong_point )
                ).str()
            );
        }
    }

    std::shared_ptr<DeviceInterface> Device() override
    {
        return device_;
    }

    virtual void StoreResults() override
    {
        SvgDocument document;
        document.SetSize( width_, height_ );
        for( const auto& line : output_data_ )
        {
            document.AddLine(
                line.x,
                line.y,
                line.z,
                line.w
            );
        }
        const std::string file_name = ( boost::format( "%1%, %2%.svg" ) %
            fixture_name_ %
            device_->Name() ).str();
        document.BuildAndWriteToDisk( file_name );
    }

    virtual ~KochCurveOpenClFixture()
    {
    }
private:
    static const int max_iterations_ = 20;
    const std::shared_ptr<OpenClDevice> device_;
    int iterations_count_;
    std::vector<T4> output_data_;
    boost::compute::program program_;
    std::vector<T4> curves_;
    double width_, height_;
    const std::string fixture_name_;

    size_t CalcLineCount()
    {
        return CalcLineCount( iterations_count_ );
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
        return CalcTotalLineCount( iterations_count_ );
    }

    std::string LineToString( const T4& line )
    {
        return ( boost::format( "(%1%; %2%) - (%3%; %4%)" ) %
            line.x %
            line.y %
            line.z %
            line.w ).str();
    }

    bool IsPointInViewport( T x, T y )
    {
        return x >= 0 && y >= 0 && x <= width_ && y <= height_;
    }
};
