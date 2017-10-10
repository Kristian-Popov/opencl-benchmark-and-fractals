#pragma once

#include "operation_step.h"
#include "benchmark_time_writer_interface.h"

class StdoutBenchmarkTimeWriter : public BenchmarkTimeWriterInterface
{
public:
    virtual void WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results ) override;
private:
    // TODO these functions are useful for other benchmark time writers. Move them to some common place?
    std::unordered_map<OperationStep, OutputDurationType> CalcAverage(
        const BenchmarkFixtureResultForOperation& perOperationData,
        const std::vector<OperationStep>& steps);
    std::unordered_map<OperationStep, std::pair<OutputDurationType, OutputDurationType>> CalcMinMax( 
        const BenchmarkFixtureResultForOperation& perOperationData,
        const std::vector<OperationStep>& steps );
};
