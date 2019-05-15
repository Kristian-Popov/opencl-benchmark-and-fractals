#include "operation_step.h"

// TODO can this be replaced with to_json?
std::string OperationStepIds::Get(OperationStep step) { return operation_serialize_ids.at(step); }

const std::unordered_map<OperationStep, std::string> OperationStepIds::operation_serialize_ids = {
    std::make_pair(OperationStep::CopyInputDataToDevice1, "CopyInputDataToDevice1"),
    std::make_pair(OperationStep::CopyInputDataToDevice2, "CopyInputDataToDevice2"),
    std::make_pair(OperationStep::CopyInputDataToDevice3, "CopyInputDataToDevice3"),
    std::make_pair(OperationStep::CopyInputDataToDevice4, "CopyInputDataToDevice4"),
    std::make_pair(OperationStep::CopyInputDataToDevice5, "CopyInputDataToDevice5"),
    std::make_pair(OperationStep::Calculate1, "Calculate1"),
    std::make_pair(OperationStep::Calculate2, "Calculate2"),
    std::make_pair(OperationStep::Calculate3, "Calculate3"),
    std::make_pair(OperationStep::Calculate4, "Calculate4"),
    std::make_pair(OperationStep::Calculate5, "Calculate5"),
    std::make_pair(OperationStep::CopyOutputDataFromDevice1, "CopyOutputDataFromDevice1"),
    std::make_pair(OperationStep::CopyOutputDataFromDevice2, "CopyOutputDataFromDevice2"),
    std::make_pair(OperationStep::CopyOutputDataFromDevice3, "CopyOutputDataFromDevice3"),
    std::make_pair(OperationStep::CopyOutputDataFromDevice4, "CopyOutputDataFromDevice4"),
    std::make_pair(OperationStep::CopyOutputDataFromDevice5, "CopyOutputDataFromDevice5"),
    std::make_pair(OperationStep::MapOutputData, "MapOutputData"),
    std::make_pair(OperationStep::UnmapOutputData, "UnmapOutputData"),
    std::make_pair(OperationStep::MultiprecisionMultiplyWords, "MultiprecisionMultiplyWords"),
    std::make_pair(OperationStep::MultiprecisionNormalize, "NormalizeNumber"),
    std::make_pair(OperationStep::CopyErrors, "CopyErrors"),
};
