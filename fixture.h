#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "operation_id.h"

#include "boost/compute/compute.hpp"

class Fixture
{
public:
    typedef std::chrono::duration<double, std::nano> Duration;
    struct ExecutionResult
    {
        Duration duration;
        OperationId operationId;

        ExecutionResult( OperationId operationId_, Duration duration_ )
            : operationId(operationId_)
            , duration(duration_)
        {}
    };

    virtual void Init() {}

	virtual std::unordered_map<OperationId, ExecutionResult> Execute( boost::compute::context& context ) = 0;

    virtual std::string Description() = 0;

    virtual ~Fixture() {}
};
