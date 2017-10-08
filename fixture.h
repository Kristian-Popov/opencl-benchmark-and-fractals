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
    struct ExecutionResult
    {
        Duration duration;
        OperationStep operationId;

        ExecutionResult( OperationStep operationId_, Duration duration_ )
            : operationId(operationId_)
            , duration(duration_)
        {}
    };

    /*
        Optional method to initialize a fixture.
        TODO Call every iteration or not? For every platform/device?
    */
    virtual void Initialize() {}

	virtual std::unordered_map<OperationStep, ExecutionResult> Execute( boost::compute::context& context ) = 0;

    /*
    Optional method to initialize a fixture.
    TODO Call every iteration or not? For every platform/device?
    */
    virtual void Finalize() {}

    virtual std::vector<OperationStep> GetSteps() = 0;

    virtual std::string Description() = 0;

    virtual ~Fixture() {}
};
