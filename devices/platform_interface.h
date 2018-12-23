#pragma once

#include <string>
#include <memory>
#include <vector>

#include "devices/device_interface.h"

class PlatformInterface
{
public:
    virtual std::string Name() = 0;
    virtual std::vector<std::shared_ptr<DeviceInterface>> GetDevices() = 0;
    virtual ~PlatformInterface() noexcept {}
};