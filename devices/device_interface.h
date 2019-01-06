#pragma once

#include <memory>
#include <string>
#include <vector>

class PlatformInterface;

class DeviceInterface
{
public:
    virtual std::string Name() = 0;
    virtual std::vector<std::string> Extensions() = 0;
    virtual std::string UniqueName() = 0;
    virtual std::weak_ptr<PlatformInterface> platform() = 0;
    virtual ~DeviceInterface() noexcept {}
};
