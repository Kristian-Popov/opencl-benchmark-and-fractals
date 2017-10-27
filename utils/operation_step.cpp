#include "operation_step.h"

std::string OperationStepDescriptionRepository::Get( OperationStep step )
{
    return operationDescriptions.at(step);
}

const std::unordered_map<OperationStep, std::string> OperationStepDescriptionRepository::operationDescriptions = {
    std::make_pair( OperationStep::CopyInputDataToDevice, "Copy input data to device" ),
    std::make_pair( OperationStep::Calculate, "Calculation" ),
    std::make_pair( OperationStep::CopyOutputDataFromDevice, "Copy output data from device" ),
    std::make_pair( OperationStep::MapOutputData, "Map output data" ),
    std::make_pair( OperationStep::UnmapOutputData, "Unmap output data" ),
};
