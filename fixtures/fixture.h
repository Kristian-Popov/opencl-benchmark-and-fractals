#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include "devices/device_interface.h"
#include "utils/duration.h"
#include "operation_step.h"

class Fixture
{
public:
    /*
        Optional method to initialize a fixture.
        Called exactly once before running a fixture.
        Memory allocations should be done here to avoid excess memory consumption since many fixtures may be created at once, 
        but only one of them will be executed at once.
    */
    virtual void Initialize() {}

    /*
    Get a list of required extensions required by this fixture
    */
    virtual std::vector<std::string> GetRequiredExtensions() = 0;

    virtual std::unordered_multimap<OperationStep, Duration> Execute() = 0;

    /*
        Optional method to finalize a fixture.
        Called exactly once after running.
    */
    virtual void Finalize() {}

    /*
        Verify execution results
        Called exactly once but for every platform/device.
    */
    virtual void VerifyResults() {}

    virtual std::shared_ptr<DeviceInterface> Device() = 0;

    virtual std::string Algorithm()
    {
        return std::string();
    }

    /*
        Store results of fixture to a persistent storage (e.g. graphic file).
        Every fixture may provide its own method, but it is optional.
    */
    virtual void StoreResults() {}

    virtual ~Fixture() noexcept {}
};
