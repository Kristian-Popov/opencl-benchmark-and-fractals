#pragma once

#include <string>
#include <memory>

class DeviceInterface
{
public:
    virtual std::string Name() = 0;
    virtual std::vector<std::string> Extensions() = 0;
    virtual std::string UniqueName() = 0;
    virtual ~DeviceInterface() noexcept {}
};
