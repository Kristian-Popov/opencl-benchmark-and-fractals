#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "operation_step.h"

#include "boost/compute/compute.hpp"

class Fixture
{
public:
    typedef std::chrono::duration<double, std::nano> Duration;

    /*
        Optional method to initialize a fixture.
        Called exactly once before running a fixture.
        Memory allocations should be done here to avoid excess memory consumption since many fixtures may be created at once, 
        but only one of them will be executed at once.
    */
    virtual void Initialize() {}

	virtual std::unordered_map<OperationStep, Duration> Execute( boost::compute::context& context ) = 0;

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

    virtual std::vector<OperationStep> GetSteps() = 0;

    virtual std::string Description() = 0;

    virtual ~Fixture() {}
};
