#pragma once

#include <string>
#include <unordered_map>

enum class OperationStep
{ 
    CopyInputDataToDevice1,
    CopyInputDataToDevice2,
    CopyInputDataToDevice3,
    CopyInputDataToDevice4,
    CopyInputDataToDevice5,
    Calculate1,
    Calculate2,
    Calculate3,
    Calculate4,
    Calculate5,
    CopyOutputDataFromDevice1,
    CopyOutputDataFromDevice2,
    CopyOutputDataFromDevice3,
    CopyOutputDataFromDevice4,
    CopyOutputDataFromDevice5,
    MapOutputData,
    UnmapOutputData,
    MultiprecisionMultiplyWords,
    MultiprecisionNormalize,
    CopyErrors
};

// TODO find a better name for this class? Or rework to a few functions in a namespace?
class OperationStepIds
{
public:
    static std::string Get( OperationStep step );
private:
    static const std::unordered_map<OperationStep, std::string> operation_serialize_ids;
};
