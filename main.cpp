#include "fixture_runner.h"

#include <iostream>

#include "stdout_benchmark_time_writer.h"
#include "html_benchmark_time_writer.h"

#include <boost/program_options.hpp>

int main( int argc, char** argv )
{
    boost::program_options::options_description desc( "Allowed options" );
    desc.add_options()
        ( "help", "produce help message" )
        ( "html", boost::program_options::value<std::string>(), "write benchmark time to HTML file" )
        ( "stdout", "write benchmark time to standard output stream. This is the default option. "
            "Disables progress printing" )
        ( "no-progress", "do not write short information about progress to standard output stream. "
            "By default this option is on if results are written to any output except standard output stream" )
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
        std::cout << "Wrong command line arguments: " << e.what() << std::endl;
        std::cout << desc << std::endl; // TODO replace with logging?
        return 1; // TODO use some error code constant?
    }
    boost::program_options::notify( vm );

    if( vm.count( "help" ) ) 
    {
        std::cout << desc << std::endl; // TODO replace with logging?
        return 1; // TODO use some error code constant?
    }
    if( vm.count( "html" ) && vm.count( "stdout" ) ) 
    {
        std::cout << "Wrong command line arguments - results can be written to only one place" << std::endl;
        std::cout << desc << std::endl; // TODO replace with logging?
        return 1; // TODO use some error code constant?
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
    FixtureRunner::Run( std::move( timeWriter ), progress );

    return 0;
}

