#pragma once

#include <stdexcept>

#include "boost/compute.hpp"

class ProgramBuildFailedException : public std::exception {
public:
    ProgramBuildFailedException(
        boost::compute::opencl_error& opencl_error, boost::compute::program& program);

    std::string DeviceName() const { return device_name_; }

    std::string Source() const { return source_; }

    std::string BuildOptions() const { return build_options; }

    std::string BuildLog() const { return build_log_; }

    virtual const char* what() const override;

private:
    std::string device_name_, source_, build_options, build_log_, error_message_;

    void BuildErrorMessage();
};
