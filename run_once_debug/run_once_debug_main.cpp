#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <cstdlib>

#include "command_line_processor.h"
#include "fixture_runner.h"
#include "run_settings.h"

int main(int argc, char** argv) {
    try {
        kpv::RunSettings settings;
        if (kpv::CommandLineProcessor::Process(argc, argv, settings)) {
            kpv::FixtureRunner fixture_runner;
            fixture_runner.Run(settings);
        }

    } catch (std::exception& e) {
        BOOST_LOG_TRIVIAL(fatal) << "Caught fatal exception: " << e.what();
        throw;
    } catch (...) {
        BOOST_LOG_TRIVIAL(fatal) << "Caught fatal error";
        throw;
    }

    return EXIT_SUCCESS;
}
