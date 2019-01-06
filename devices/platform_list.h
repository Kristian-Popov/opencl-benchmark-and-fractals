#pragma once

#include <memory>
#include <vector>

#include <boost/compute.hpp>

#include "devices/opencl_platform.h"
#include "devices/platform_interface.h"

class PlatformList
{
public:
    PlatformList()
    {
        std::vector<boost::compute::platform> opencl_platforms = boost::compute::system::platforms();
        opencl_platforms_.reserve( opencl_platforms.size() );
        for( boost::compute::platform& platform : opencl_platforms )
        {
            auto& ptr = std::make_shared<OpenClPlatform>( platform );
            ptr->Initialize();
            opencl_platforms_.push_back( ptr );
        }
    }

    std::vector<std::shared_ptr<PlatformInterface>> OpenClPlatforms()
    {
        return opencl_platforms_;
    }
private:
    std::vector<std::shared_ptr<PlatformInterface>> opencl_platforms_;
};