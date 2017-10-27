#pragma once

#include <unordered_map>

enum class OperationStep
{ 
    CopyInputDataToDevice,
    Calculate,
    CopyOutputDataFromDevice,
    MapOutputData,
    UnmapOutputData
};

class OperationStepDescriptionRepository
{
public:
    static std::string Get( OperationStep step);
private:
    static const std::unordered_map<OperationStep, std::string> operationDescriptions;
};
