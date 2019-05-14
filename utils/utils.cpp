#include "utils.h"

#include "program_build_failed_exception.h"

#include <type_traits>

#include <boost/log/trivial.hpp>
#include <boost/format.hpp>

namespace Utils
{
    std::string ReadFile( const std::string& fileName )
    {
        std::ifstream stream( fileName );
        std::stringbuf buf;
        if( !stream.is_open() )
        {
            throw std::runtime_error( "File not found" );
        }
        stream >> &buf;
        stream.close();
        return buf.str();
    }

    std::string FormatQuantityString( int value )
    {
        std::string result;
        if (value % 1000000 == 0)
        {
            result = std::to_string(value / 1000000) + "M";
        }
        else if( value % 1000 == 0 )
        {
            result = std::to_string( value / 1000 ) + "K";
        }
        else
        {
            result = std::to_string( value);
        }
        return result;
    }

    // Based on this article
    // https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
    template<typename T>
    bool AreFloatValuesClose( T A, T B,
        T maxAbsDiff, T maxRelDiff ) // TODO not sure about of these differences - T or e.g. long double?
    {
        //TODO disabled since it doesn't work for half precision
        //static_assert( std::is_floating_point<T>::value, "AreFloatValuesClose function works only for floating point types" );
        // Check if the numbers are really close -- needed
        // when comparing numbers near zero.
        long double diff = fabs( A - B );
        if( diff <= maxAbsDiff )
        {
            return true;
        }

        A = fabs( A );
        B = fabs( B );
        T largest = std::max( A, B );

        return diff <= largest * maxRelDiff;
    }

    // TODO rework to a more convenient class?
    long double ChooseConvenientUnit( long double value,
        const std::vector<long double>& units )
    {
        EXCEPTION_ASSERT( !units.empty() );
        EXCEPTION_ASSERT( std::is_sorted( units.begin(), units.end() ) );
        // Find the biggest unit that is smaller than input value
        auto resultIter = std::find_if( units.crbegin(), units.crend(),
            [value] (long double unit )
            {
                return unit < value;
            } );
        long double result = 1.0f;
        if ( resultIter == units.crend() )
        {
            result = *units.begin();
        }
        else
        {
            result = *resultIter;
        }
        return result;
    }

    long double ChooseConvenientUnit( const std::vector<long double>& values,
        const std::vector<long double>& units )
    {
        std::vector<long double> conv_units;
        conv_units.reserve( values.size() );

        std::transform( values.begin(), values.end(), std::back_inserter( conv_units ),
            [&units] (long double value)
            {
                return ChooseConvenientUnit(value, units);
            } );

        // Count how many times every unit occurs
        std::unordered_map<long double, int> counts;
        for( long double unit: conv_units )
        {
            ++counts[unit];
        }
        EXCEPTION_ASSERT( !counts.empty() );
        // TODO find a unit that occurs most frequently
        auto resultIter = std::max_element( counts.begin(), counts.end(),
            CompareSecond<long double, int> );
        return resultIter->first;
    }

    boost::compute::program BuildProgram( boost::compute::context& context,
        const std::string& source,
        const std::string& buildOptions /*= std::string()*/,
        const std::vector<std::string>& extensions /*= std::vector<std::string>()*/ )
    {
        std::string combined_source;
        for( const std::string& extension : extensions )
        {
            std::string f;
            // TODO implement a more generic solutions for optional extensions
            // that became optional core features
            if( extension == "cl_khr_fp64" )
            {
                f = R"(#if __OPENCL_VERSION__ <= CL_VERSION_1_1
    #pragma OPENCL EXTENSION %1% : enable
#endif
)";
            }
            else
            {
                f = "#pragma OPENCL EXTENSION %1% : enable\n";
            }
            combined_source += ( boost::format( f ) % extension ).str();
        }
        combined_source += source;

        // Taken from boost::compute::program::create_with_source() so we have build log
        // left in case of errors
        const char *source_string = combined_source.c_str();

        cl_int error = 0;
        cl_program program_ = clCreateProgramWithSource( context,
            cl_uint( 1 ),
            &source_string,
            0,
            &error );
        boost::compute::program program;
        try
        {
            if( !program_ )
            {
                throw boost::compute::opencl_error( error );
            }
            program = boost::compute::program( program_ );
            program.build( buildOptions );
            return program;
        }
        catch( boost::compute::opencl_error& error )
        {
            if( error.error_code() == CL_BUILD_PROGRAM_FAILURE )
            {
                throw ProgramBuildFailedException( error, program );
            }
            else
            {
                throw;
            }
        }
    }

    boost::compute::kernel BuildKernel( const std::string& name,
        boost::compute::context& context,
        const std::string& source,
        const std::string& buildOptions,
        const std::vector<std::string>& extensions)
    {
        return boost::compute::kernel( BuildProgram( context, source, buildOptions, extensions), name );
    }

    std::string CombineStrings( const std::vector<std::string>& strings, const std::string & delimiter )
    {
        return VectorToString( strings, delimiter );
    }

    cl_double4 CombineTwoDouble2Vectors( const cl_double2 & a, const cl_double2 & b )
    {
        return { a.x, a.y, b.x, b.y };
    }

    std::unordered_map<OperationStep, Duration> GetOpenCLEventDurations(
        const std::unordered_map<OperationStep, boost::compute::event>& events )
    {
        std::unordered_map<OperationStep, Duration> result;
        for( const std::pair<OperationStep, boost::compute::event>& p : events )
        {
            result.emplace( p.first, Duration( p.second ) );
        }
        return result;
    }
}
