#include "program_build_failed_exception.h"

#include <sstream>

#include "utils.h"

ProgramBuildFailedException::ProgramBuildFailedException( boost::compute::opencl_error& openclError,
    boost::compute::program& program )
{
    EXCEPTION_ASSERT( openclError.error_code() == CL_BUILD_PROGRAM_FAILURE );

    try // Retrieving these values can cause OpenCL errors, so catch them to write at least something about an error
    {
        deviceName_ = program.get_devices().front().name();
        buildLog_ = program.build_log();
        buildOptions_ = program.get_build_info<std::string>( CL_PROGRAM_BUILD_OPTIONS,
            program.get_devices().front() );
        source_ = program.source();

        BuildErrorMessage();
    }
    catch( boost::compute::opencl_error& )
    {
    }
}

void ProgramBuildFailedException::BuildErrorMessage()
{
    std::stringstream stream;
    stream << "Program failed to build on device " <<
        DeviceName() << std::endl <<
        "Kernel source: " << std::endl << Source() << std::endl <<
        "Build options: " << BuildOptions() << std::endl <<
        "Build log: " << BuildLog() << std::endl;
    errorMessage_ = stream.str();
}

const char* ProgramBuildFailedException::what() const
{
    return errorMessage_.c_str();
}