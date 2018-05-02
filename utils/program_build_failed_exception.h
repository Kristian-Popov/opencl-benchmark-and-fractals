#pragma once

#include <stdexcept>

#include "boost/compute.hpp"

class ProgramBuildFailedException: public std::exception
{
public:
    ProgramBuildFailedException( boost::compute::opencl_error& openclError, boost::compute::program& program );

    std::string DeviceName() const
    {
        return deviceName_;
    }

    std::string Source() const
    {
        return source_;
    }

    std::string BuildOptions() const
    {
        return buildOptions_;
    }

    std::string BuildLog() const
    {
        return buildLog_;
    }

    virtual const char* what() const override;
private:
    std::string deviceName_, source_, buildOptions_, buildLog_, errorMessage_;

    void BuildErrorMessage();
};