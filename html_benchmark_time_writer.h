#pragma once

#include <memory>

#include "benchmark_time_writer_interface.h"
#include "html_document.h"

class HTMLBenchmarkTimeWriter: public BenchmarkTimeWriterInterface
{
public:
    HTMLBenchmarkTimeWriter( const char* fileName );

    virtual void WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results ) override;

    virtual void Flush() override;

    std::shared_ptr<HTMLDocument> GetHTMLDocument()
    {
        return document_;
    }
private:
    std::shared_ptr<HTMLDocument> document_;

    void AddOperationsResultsToRow(
        const BenchmarkFixtureResultForDevice& deviceData,
        const std::vector<OperationStep>& steps,
        std::vector<HTMLDocument::CellDescription>& row );
    std::unordered_map<OperationStep, OutputDurationType> CalcAverage(
        const BenchmarkFixtureResultForOperation& perOperationData,
        const std::vector<OperationStep>& steps );
};
