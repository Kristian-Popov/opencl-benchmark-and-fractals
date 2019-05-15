#pragma once

#include <boost/compute.hpp>
#include <memory>
#include <vector>

#include "devices/opencl_platform.h"
#include "devices/platform_interface.h"

class PlatformList {
public:
    PlatformList() {
        std::vector<boost::compute::platform> opencl_platforms =
            boost::compute::system::platforms();
        opencl_platforms_.reserve(opencl_platforms.size());
        for (boost::compute::platform& platform : opencl_platforms) {
            auto& ptr = std::make_shared<OpenClPlatform>(platform);
            ptr->Initialize();
            all_platforms_.push_back(ptr);
            opencl_platforms_.push_back(ptr);
        }
        // TODO add host platform
    }

    std::vector<std::shared_ptr<PlatformInterface>> OpenClPlatforms() const {
        return opencl_platforms_;
    }

    std::vector<std::shared_ptr<PlatformInterface>> AllPlatforms() const { return all_platforms_; }

private:
    std::vector<std::shared_ptr<PlatformInterface>> all_platforms_;
    std::vector<std::shared_ptr<PlatformInterface>> opencl_platforms_;
};
