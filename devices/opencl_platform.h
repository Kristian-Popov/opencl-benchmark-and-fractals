#pragma once

#include "boost/compute.hpp"
#include "devices/opencl_device.h"
#include "devices/platform_interface.h"

class OpenClPlatform : public PlatformInterface,
                       public std::enable_shared_from_this<OpenClPlatform> {
public:
    OpenClPlatform(const boost::compute::platform& openclPlatform)
        : opencl_platform_(openclPlatform) {}

    void Initialize() {
        std::vector<boost::compute::device> opencl_devices = opencl_platform_.devices();
        devices_.reserve(opencl_devices.size());
        for (boost::compute::device& device : opencl_devices) {
            devices_.push_back(std::make_shared<OpenClDevice>(device, shared_from_this()));
        }
    }

    std::string Name() override { return opencl_platform_.name(); }

    std::vector<std::shared_ptr<DeviceInterface>> GetDevices() override {
        return std::vector<std::shared_ptr<DeviceInterface>>(devices_.cbegin(), devices_.cend());
    }

private:
    boost::compute::platform opencl_platform_;
    std::vector<std::shared_ptr<OpenClDevice>> devices_;
};
