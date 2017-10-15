#include "fixture_runner.h"

#include <iostream>

#include "stdout_benchmark_time_writer.h"
#include "html_benchmark_time_writer.h"
#include <cstdlib>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>

int main( int argc, char** argv )
{
    namespace log = boost::log::trivial;

    boost::program_options::options_description desc( "Allowed options" );
    desc.add_options()
        ( "help", "produce help message" )
        ( "html", boost::program_options::value<std::string>(), "write benchmark time to HTML file" )
        ( "stdout", "write benchmark time to standard output stream. This is the default option. "
            "Disables progress printing" )
        ( "no-progress", "do not write short information about progress to standard output stream. "
            "By default this option is on if results are written to any output except standard output stream" )
        ( "no-trivial-factorial", "do not run a trivial factorial fixture. " )
        ( "no-damped-wave2d", "do not run a damped wave in 2D fixture. " )
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
    boost::program_options::notify( vm );

    if( vm.count( "help" ) ) 
    {
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }
    if( vm.count( "html" ) && vm.count( "stdout" ) ) 
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Wrong command line arguments - results can be written to only one place";
        BOOST_LOG_TRIVIAL( fatal ) << desc;
        return EXIT_FAILURE;
    }

    bool progress = vm.count( "no-progress" ) == 0;
    std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter;
    if( vm.count( "html" ) )
    {
        timeWriter = std::make_unique<HTMLBenchmarkTimeWriter>( vm["html"].as<std::string>().c_str() );
    }
    else // This is the default option
    {
        timeWriter = std::make_unique<StdoutBenchmarkTimeWriter>();
        progress = false;
    }
    log::severity_level minSeverityToIgnore = progress ? log::debug : log::fatal;
    boost::log::core::get()->set_filter
    (
        log::severity > minSeverityToIgnore
    );

    FixtureRunner::FixturesToRun fixturesToRun;
    fixturesToRun.trivialFactorial = vm.count( "no-trivial-factorial" ) == 0;
    fixturesToRun.dampedWave2D = vm.count( "no-damped-wave2d" ) == 0;
    FixtureRunner::Run( std::move( timeWriter ), fixturesToRun );

    return 0;
}

