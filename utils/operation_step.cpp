#include "operation_step.h"

std::string OperationStepDescriptionRepository::Get( OperationStep step )
{
    auto iter = operationDescriptions.find( step );
    if( iter != operationDescriptions.cend() )
    {
        return iter->second;
    }
    else
    {
        return "Unknown operation";
    }
}

std::string OperationStepDescriptionRepository::GetSerializeId( OperationStep step )
{
    return operation_serialize_ids.at( step );
}

const std::unordered_map<OperationStep, std::string> OperationStepDescriptionRepository::operationDescriptions = {
    std::make_pair( OperationStep::CopyInputDataToDevice, "Copy input data to device" ),
    std::make_pair( OperationStep::Calculate, "Calculation" ),
    std::make_pair( OperationStep::CopyOutputDataFromDevice, "Copy output data from device" ),
    std::make_pair( OperationStep::MapOutputData, "Map output data" ),
    std::make_pair( OperationStep::UnmapOutputData, "Unmap output data" ),
    std::make_pair( OperationStep::MultiprecisionMultiplyWords, "Multiply words" ),
    std::make_pair( OperationStep::MultiprecisionNormalize, "Normalize number" ),
    std::make_pair( OperationStep::CopyErrors, "Copy runtime errors" ),
};

const std::unordered_map<OperationStep, std::string> OperationStepDescriptionRepository::operation_serialize_ids = {
    std::make_pair( OperationStep::CopyInputDataToDevice, "CopyInputDataToDevice" ),
    std::make_pair( OperationStep::Calculate, "Calculate" ),
    std::make_pair( OperationStep::CopyOutputDataFromDevice, "CopyOutputDataFromDevice" ),
    std::make_pair( OperationStep::MapOutputData, "MapOutputData" ),
    std::make_pair( OperationStep::UnmapOutputData, "UnmapOutputData" ),
    std::make_pair( OperationStep::MultiprecisionMultiplyWords, "MultiprecisionMultiplyWords" ),
    std::make_pair( OperationStep::MultiprecisionNormalize, "NormalizeNumber" ),
    std::make_pair( OperationStep::CopyErrors, "CopyErrors" ),
};
