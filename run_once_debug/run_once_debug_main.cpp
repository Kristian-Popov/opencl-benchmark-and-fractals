#include "fixture_runner.h"

#include <iostream>

#include "reporters/json_benchmark_reporter.h"
#include "utils/utils.h"
#include <cstdlib>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>

namespace
{
    static const std::string defaultOutputFileName( "output.json" );
}

int main( int argc, char** argv )
{
    namespace log = boost::log::trivial;

    boost::program_options::options_description desc( "Allowed options" );
    desc.add_options()
        ( "help", "produce help message" )
        ( "no-progress", "do not write short information about progress to standard output stream. "
            "By default this option is on." )
        ( "no-trivial-factorial", "do not run a trivial factorial fixture. " )
        ( "no-damped-wave2d", "do not run a damped wave in 2D fixture. " )
        ( "no-koch-curve", "do not run a Koch curve fixture. " )
        ( "no-multibrot-set", "do not run a Mandelbrot/Multibrot set fixture. " )
        ( "no-multiprecision-factorial", "do not run multiprecision factorial fixture. " )
        ;

    // TODO something is wrong here. For command line arguments line "--html --stdout" only "html" part is present in vm after parsing
    // Also possible that in this case an empty file called "--stdout" is created
    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store( boost::program_options::parse_command_line( argc, argv, desc ), vm );
    }
    catch( boost::program_options::invalid_command_line_syntax& e )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Wrong command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }
    catch (std::exception& e)
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught exception when parsin command line arguments: " << e.what();
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }
    boost::program_options::notify( vm );

    if( vm.count( "help" ) )
    {
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    bool progress = vm.count( "no-progress" ) == 0;
    std::string outputFileName = defaultOutputFileName;

    std::unique_ptr<BenchmarkReporter> timeWriter = std::make_unique<JsonBenchmarkReporter>( outputFileName );

    log::severity_level minSeverityToIgnore = progress ? log::debug : log::fatal;
    boost::log::core::get()->set_filter
    (
        // TODO add super verbose mode
        log::severity > minSeverityToIgnore
    );

    FixtureRunner::FixturesToRun fixturesToRun;
    fixturesToRun.trivialFactorial = vm.count( "no-trivial-factorial" ) == 0;
    fixturesToRun.dampedWave2D = vm.count( "no-damped-wave2d" ) == 0;
    fixturesToRun.kochCurve = vm.count( "no-koch-curve" ) == 0;
    fixturesToRun.multibrotSet = vm.count( "no-multibrot-set" ) == 0;
    fixturesToRun.multiprecisionFactorial = vm.count( "no-multiprecision-factorial" ) == 0;

    FixtureRunner::RunSettings settings;
    settings.fixturesToRun = fixturesToRun;
    settings.debug_mode = true;
    settings.minIterations = 1;
    settings.maxIterations = 1;

    // TODO fill the rest of properties

    try
    {
        FixtureRunner fixtureRunner;
        EXCEPTION_ASSERT( timeWriter );
        fixtureRunner.Run( std::move( timeWriter ), settings );
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

