#pragma once

#include <string>
#include <unordered_map>

enum class OperationStep
{ 
    CopyInputDataToDevice,
    Calculate,
    CopyOutputDataFromDevice,
    MapOutputData,
    UnmapOutputData,
    MultiprecisionMultiplyWords,
    MultiprecisionNormalize,
    CopyErrors
};

// TODO find a better name for this class? Or rework to a few functions in a namespace?
class OperationStepDescriptionRepository
{
public:
    static std::string Get( OperationStep step );
    static std::string GetSerializeId( OperationStep step );
private:
    static const std::unordered_map<OperationStep, std::string> operationDescriptions;
    static const std::unordered_map<OperationStep, std::string> operation_serialize_ids;
};
