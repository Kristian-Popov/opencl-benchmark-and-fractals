#include "fixture_runner.h"

#include <cstdlib>
#include <iostream>

#include "fixture_command_line_processor.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/program_options.hpp>

#include "utils/utils.h"

int main( int argc, char** argv )
{
    try
    {
        FixtureRunner::RunSettings settings;
        auto reporter = FixtureCommandLineProcessor::Process( argc, argv, settings );
        if( reporter )
        {
            settings.debug_mode = true;
            settings.min_iterations = 1;
            settings.max_iterations = 1;

            FixtureRunner fixture_runner;
            fixture_runner.Run( std::move( reporter ), settings );
        }
    }
    catch( std::exception& e )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught fatal exception: " << e.what();
        throw;
    }
    catch( ... )
    {
        BOOST_LOG_TRIVIAL( fatal ) << "Caught fatal error";
        throw;
    }

    return EXIT_SUCCESS;
}