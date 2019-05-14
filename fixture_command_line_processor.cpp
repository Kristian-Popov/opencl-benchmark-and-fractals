#include "fixture_command_line_processor.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>

namespace
{
    static const char* const kDefaultOutputFileName = "output.json";
}

std::unique_ptr<JsonBenchmarkReporter> FixtureCommandLineProcessor::Process(
    int argc, char** argv, FixtureRunner::RunSettings& settings
)
{
    namespace log = boost::log::trivial;

    boost::program_options::options_description desc( "Allowed options" );
    desc.add_options()
        ( "help,h", "produce help message" )
        ( "verbose,v", "verbose mode (more output is written to the console)" )
        ( "version,V", "print version" )
        ( "no-trivial-factorial", "do not run a trivial factorial fixture. " )
        ( "no-damped-wave2d", "do not run a damped wave in 2D fixture. " )
        ( "no-koch-curve", "do not run a Koch curve fixture. " )
        ( "no-multibrot-set", "do not run a Mandelbrot/Multibrot set fixture. " )
        ;

    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store( boost::program_options::parse_command_line( argc, argv, desc ), vm );
    }
    catch( boost::program_options::invalid_command_line_syntax& e )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Wrong command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        throw;
    }
    catch( std::exception& e )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught exception when parsing command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        throw;
    }
    boost::program_options::notify( vm );

    if( vm.count( "help" ) )
    {
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        throw;
    }

    if( vm.count( "version" ) )
    {
        PrintVersion();
        return std::unique_ptr<JsonBenchmarkReporter>();
    }

    log::severity_level min_severity = ( vm.count( "verbose" ) > 0 ) ? log::trace : log::info;
    boost::log::core::get()->set_filter
    (
        log::severity >= min_severity
    );

    FixtureRunner::FixturesToRun fixtures_to_run;
    fixtures_to_run.trivial_factorial = vm.count( "no-trivial-factorial" ) == 0;
    fixtures_to_run.damped_wave = vm.count( "no-damped-wave2d" ) == 0;
    fixtures_to_run.koch_curve = vm.count( "no-koch-curve" ) == 0;
    fixtures_to_run.multibrot_set = vm.count( "no-multibrot-set" ) == 0;

    settings.fixtures_to_run = fixtures_to_run;
    settings.target_fixture_execution_time = Duration( std::chrono::milliseconds( 100 ) );
    // Leave default values for the rest of parameters

    std::string output_file_name = kDefaultOutputFileName;
    return std::make_unique<JsonBenchmarkReporter>( output_file_name );
}

void FixtureCommandLineProcessor::PrintVersion()
{
    BOOST_LOG_TRIVIAL( info ) << "Version is 0.1.0";
}