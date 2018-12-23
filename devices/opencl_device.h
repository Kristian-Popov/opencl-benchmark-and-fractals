#pragma once

#include <boost/compute.hpp>

#include "devices/device_interface.h"

class OpenClDevice: public DeviceInterface
{
public:
    OpenClDevice( const boost::compute::device& computeDevice )
        : device_( computeDevice )
        , context_( computeDevice )
        , queue_( context_, computeDevice, boost::compute::command_queue::enable_profiling )
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

    std::vector<std::string> Extensions() override
    {
        return device_.extensions();
    }

    std::string UniqueName() override
    {
        // TODO make sure it is unique
        return Name();
    }

private:
    boost::compute::device device_;
    boost::compute::context context_;
    boost::compute::command_queue queue_;
};
