#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>

#include "coordinate_formatter.h"
#include "utils/utils.h"

#include "multibrot_opencl/multibrot_parallel_calculator.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>
#include "lodepng/source/lodepng.h"

#ifdef WIN32
#   include <windows.h>
#   include <tchar.h>
#endif

namespace
{
    constexpr const char* kDefaultSizePix = "10000x10000";
    constexpr const char* kFirstPhaseTempFileRegexpr = R"(^multibrot_\d+_\d+\.png$)";
    constexpr const char* kRowTempFileRegexpr = R"(row_\d+\.png)";

    template<typename P>
    struct Constants
    {
        static const LodePNGColorType pixel_format;
        static const unsigned bit_depth;
    };

    const LodePNGColorType Constants<cl_uchar>::pixel_format = LCT_GREY;
    const unsigned Constants<cl_uchar>::bit_depth = 8;

    const LodePNGColorType Constants<cl_ushort>::pixel_format = LCT_GREY;
    const unsigned Constants<cl_ushort>::bit_depth = 16;

    const LodePNGColorType Constants<cl_uchar4>::pixel_format = LCT_RGBA;
    const unsigned Constants<cl_uchar4>::bit_depth = 8;

    const LodePNGColorType Constants<cl_ushort4>::pixel_format = LCT_RGBA;
    const unsigned Constants<cl_ushort4>::bit_depth = 16;

#ifdef WIN32
    boost::filesystem::path FindTempDirectoryWin()
    {
        TCHAR temp_buffer[MAX_PATH + 1];
        GetTempPathA( MAX_PATH + 1, temp_buffer );
        return boost::filesystem::path{temp_buffer};
    }
#endif

    // Find a folder dedicated to temporary files
    // TODO move this to a separate class or contribute to another library?
    boost::filesystem::path FindTempDirectory()
    {
#ifdef UNIX
        return "/tmp/multibrot_console/";
#elif defined WIN32
        // Cache it, it may be relatively expensive
        static boost::filesystem::path temp_dir = FindTempDirectoryWin();
        return temp_dir / "multibrot_console";
#else
        // Use a subfolder in current folder on other systems
        return "./multibrot_console/";
#endif
    }

    void RemoveTemporaryFiles( const char* regexpr )
    {
        std::regex regex{regexpr};
        for( auto& dir : boost::filesystem::directory_iterator( FindTempDirectory() ) )
        {
            if( std::regex_match( dir.path().filename().string(), regex ) )
            {
                BOOST_LOG_TRIVIAL( debug ) << "Removing file " << dir.path();
                if( !boost::filesystem::remove( dir.path() ) )
                {
                    BOOST_LOG_TRIVIAL( error ) << "Attempt to remove temporary file " << dir.path() << " that didn't exist in the first place";
                }
            }
        }
    }

    // TODO move this to a separate class and remove temp folder in its destructor
    void PrepareTempFolder()
    {
        using namespace boost::filesystem;
        const path temp_folder = FindTempDirectory();
        if( exists( temp_folder ) )
        {
            if( !is_directory( temp_folder ) )
            {
                throw std::runtime_error( "Temporary folder name is taken by another file." );
            }
        }
        else
        {
            create_directories( temp_folder );
        }
    }

    template<typename P>
    void Execute( size_t total_width, size_t total_height, double power, int max_iterations )
    {
        CoordinateFormatter formatter{total_width, total_height};

        std::complex<double> min{-2.5, -2.0};
        std::complex<double> max{1.5, 2.0};
        MultibrotParallelCalculator<P> calculator{
            total_width, total_height, min, max, power, max_iterations
        };

        PrepareTempFolder();
        BOOST_LOG_TRIVIAL( info ) << "Deleting left-over temporary files from previous operation "
            "multibrot_*_xxxxx.png";
        RemoveTemporaryFiles( kFirstPhaseTempFileRegexpr );

        BOOST_LOG_TRIVIAL( info ) << "Deleting left-over temporary files from previous operation "
            "row_xxxxx.png";
        RemoveTemporaryFiles( kRowTempFileRegexpr );

        // TODO make these cached data structures thread-safe?
        // List of segments stored by their y coordinates
        // TODO is storing segments is needed at all?
        std::unordered_map<size_t, std::vector<ImagePartitioner::Segment>> segments;
        const boost::filesystem::path temp_folder{FindTempDirectory()};

        calculator.Calculate( [&](
            const boost::compute::device& device,
            const ImagePartitioner::Segment& segment,
            const P* result )
        {
            auto segment_vector_iter = segments.find( segment.y );
            if( segment_vector_iter == segments.end() )
            {
                segment_vector_iter = segments.insert(
                    std::make_pair( segment.y, std::vector<ImagePartitioner::Segment>() ) ).first;
            }
            segment_vector_iter->second.push_back( segment );

            std::string filename = ( boost::format( "multibrot_%1%_%2%.png" ) %
                formatter.Format( segment.x ) % formatter.Format( segment.y )
                ).str();
            unsigned error = lodepng::encode( ( temp_folder / filename ).string(),
                reinterpret_cast<const unsigned char*>( result ),
                segment.width_pix, segment.height_pix,
                Constants<P>::pixel_format,
                Constants<P>::bit_depth
            );
            if( error )
            {
                BOOST_LOG_TRIVIAL( info ) << "Error when building PNG based on data by device " << device.name() << " with size " <<
                    segment.width_pix << "x" << segment.height_pix
                    << " pixels.";
            }
            BOOST_LOG_TRIVIAL( info ) << "Batch on device " << device.name() << " with size " <<
                segment.width_pix << "x" << segment.height_pix
                << " pixels finished.";
        } );

        for( auto& row : segments )
        {
            // TODO change call to ImageMagick by Magick++ library, that way there's no need
            // to call dangerous method system(), installing it beforehand
            // and possibly can be done in parallel
            BOOST_LOG_TRIVIAL( info ) << "Row building error code is " <<
                system( ( boost::format( "magick montage %1%/multibrot_*_%2%.png -tile x1 -mode Concatenate "
                    "%1%/row_%2%.png" ) %
                    FindTempDirectory() %
                    formatter.Format( row.first )
                    ).str().c_str() );
        }

        BOOST_LOG_TRIVIAL( info ) << "Deleting temporary files multibrot_*_xxxxx.png";
        RemoveTemporaryFiles( kFirstPhaseTempFileRegexpr );

        BOOST_LOG_TRIVIAL( info ) << "Started building result image, it may take a while";
        BOOST_LOG_TRIVIAL( info ) << "Result building error code is " <<
            system( ( boost::format(
                "magick montage %1%/row_*.png -tile 1x -mode Concatenate multibrot.png" ) %
                FindTempDirectory()
                ).str().c_str() );

        BOOST_LOG_TRIVIAL( info ) << "Deleting temporary files row_xxxxx.png";
        RemoveTemporaryFiles( kRowTempFileRegexpr );
    }
}

int main( int argc, char** argv )
{
    namespace log = boost::log::trivial;

    double power = 2.0;
    int bitdepth = 8;
    bool color = true;
    std::string size_pix;
    int max_iterations = 255;
    boost::program_options::options_description desc( "Multibrot set plotter - console version." );
    {
        using namespace boost::program_options;
        desc.add_options()
            ( "help", "produce help message" )
            ( "verbose", "verbose mode (more output is written to the console)" )
            ( "size,s", value<std::string>(&size_pix)->default_value( std::string( kDefaultSizePix ) ),
                "image size in pixels, should be given separated by x. Default is 10000x10000 "
                "(ten thousand by ten thousand pixels)." )
            ( "power,p", value<double>(&power)->default_value( 2.0 ), "complex number power used during plotting. "
                "May be any real positive number "
                "(negative values are possible but not supported at a moment). Use 2 for a classic Mandelbrot set. "
                "Default is 2" )
            ( "color,c", "build a color image. This is the default value." )
            ( "grayscale,g", "build a grayscale image. Mutually exclusive with --color" )
            ( "bitdepth,b", value<int>(&bitdepth)->default_value( 8 ), "image bit depth. Values values are 8 are 16. Default value is 8" )
            ( "max-iterations,m", value<int>( &max_iterations )->default_value( 255 ), "max iteration number. Must be less or equal to 255 "
                "if bit depth is 8, or less or equal to 2^16 (65535) if bit depth is 16. Default value is 255." )
            ;
    }

    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store( boost::program_options::parse_command_line( argc, argv, desc ), vm );
    }
    catch( boost::program_options::invalid_command_line_syntax& e )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Invalid command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }
    catch( std::exception& e )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught exception when parsing command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }
    boost::program_options::notify( vm );

    if( vm.count( "help" ) )
    {
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    if( vm.count( "verbose" ) )
    {
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    log::severity_level min_severity = ( vm.count( "verbose" ) > 0 ) ? log::trace : log::info;
    boost::log::core::get()->set_filter
    (
        log::severity >= min_severity
    );

    // TODO support negative powers
    if( power < 0 )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Sorry, negative powers are not yet supported.";
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    size_t total_width = 10000;
    size_t total_height = 10000;
    {
        // Parse requested image size
        size_t x_pos = 0;
        total_width = std::stoull( size_pix, &x_pos );
        if( size_pix.at( x_pos ) != 'x' && size_pix.at( x_pos ) != 'X' )
        {
            BOOST_LOG_TRIVIAL( fatal ) << "Size parameter has wrong format.";
            BOOST_LOG_TRIVIAL( fatal ) << desc;
            return EXIT_FAILURE;
        }
        total_height = std::strtoull( size_pix.c_str() + x_pos + 1, nullptr, 10 );
    }

    if( vm.count( "grayscale" ) && vm.count( "color" ) )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Given mutually exclusive options --grayscale and --color parameters. "
            "Select one of them.";
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    color = vm.count( "grayscale" ) == 0;

    if( bitdepth != 8 && bitdepth != 16 )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Given incoorect bitdepth " << bitdepth <<
            ", the only acceptable value are 8 and 16.";
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    try
    {
        if( color )
        {
            if( bitdepth == 8 )
            {
                Execute<cl_uchar4>( total_width, total_height, power, max_iterations );
            }
            else if( bitdepth == 16 )
            {
                Execute<cl_ushort4>( total_width, total_height, power, max_iterations );
            }
        }
        else
        {
            if( bitdepth == 8 )
            {
                Execute<cl_uchar>( total_width, total_height, power, max_iterations );
            }
            else if( bitdepth == 16 )
            {
                Execute<cl_ushort>( total_width, total_height, power, max_iterations );
            }
        }
    }
    catch(std::exception& e)
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught fatal exception: " << e.what();
        throw;
    }
    catch(...)
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught fatal error";
        throw;
    }

    return EXIT_SUCCESS;
}

