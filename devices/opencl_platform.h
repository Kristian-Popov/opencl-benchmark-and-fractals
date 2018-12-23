#pragma once

#include "boost/compute.hpp"

#include "devices/platform_interface.h"
#include "devices/opencl_device.h"

class OpenClPlatform: public PlatformInterface
{
public:
    OpenClPlatform( const boost::compute::platform& openclPlatform )
        : opencl_platform_( openclPlatform )
    {
        std::vector<boost::compute::device> opencl_devices = opencl_platform_.devices();
        devices_.reserve( opencl_devices.size() );
        for( boost::compute::device& device: opencl_devices )
        {
            devices_.push_back( std::make_shared<OpenClDevice>( device ) );
        }
    }

    std::string Name() override
    {
        return opencl_platform_.name();
    }

    std::vector<std::shared_ptr<DeviceInterface>> GetDevices() override
    {
        return std::vector<std::shared_ptr<DeviceInterface>>( devices_.cbegin(), devices_.cend() );
    }

private:
    boost::compute::platform opencl_platform_;
    std::vector<std::shared_ptr<OpenClDevice>> devices_;
};