#pragma once

#include <boost/compute.hpp>

#include "devices/device_interface.h"

class OpenClDevice: public DeviceInterface
{
public:
    OpenClDevice( const boost::compute::device& computeDevice, std::weak_ptr<PlatformInterface> platform )
        : device_( computeDevice )
        , context_( computeDevice )
        , queue_( context_, computeDevice, boost::compute::command_queue::enable_profiling )
        , platform_( platform )
    {
    }

    virtual std::string Name() override
    {
        return device_.name();
    }

    boost::compute::context& GetContext()
    {
        return context_;
    }

    boost::compute::command_queue& GetQueue()
    {
        return queue_;
    }

    boost::compute::device& device()
    {
        return device_;
    }

    std::vector<std::string> Extensions() override
    {
        return device_.extensions();
    }

    std::string UniqueName() override
    {
        // TODO make sure it is unique
        return Name();
    }

    std::weak_ptr<PlatformInterface> platform()
    {
        return platform_;
    }
private:
    boost::compute::device device_;
    boost::compute::context context_;
    boost::compute::command_queue queue_;
    std::weak_ptr<PlatformInterface> platform_;
};
