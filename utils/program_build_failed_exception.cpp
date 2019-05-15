#include "program_build_failed_exception.h"

#include <sstream>

#include "utils.h"

ProgramBuildFailedException::ProgramBuildFailedException(
    boost::compute::opencl_error& opencl_error, boost::compute::program& program) {
    EXCEPTION_ASSERT(opencl_error.error_code() == CL_BUILD_PROGRAM_FAILURE);

    try  // Retrieving these values can cause OpenCL errors, so catch them to write at least
         // something about an error
    {
        device_name_ = program.get_devices().front().name();
        build_log_ = program.build_log();
        build_options = program.get_build_info<std::string>(
            CL_PROGRAM_BUILD_OPTIONS, program.get_devices().front());
        source_ = program.source();

        BuildErrorMessage();
    } catch (boost::compute::opencl_error&) {
    }
}

void ProgramBuildFailedException::BuildErrorMessage() {
    std::stringstream stream;
    stream << "Program failed to build on device " << DeviceName() << std::endl
           << "Kernel source: " << std::endl
           << Source() << std::endl
           << "Build options: " << BuildOptions() << std::endl
           << "Build log: " << BuildLog() << std::endl;
    error_message_ = stream.str();
}

const char* ProgramBuildFailedException::what() const { return error_message_.c_str(); }
