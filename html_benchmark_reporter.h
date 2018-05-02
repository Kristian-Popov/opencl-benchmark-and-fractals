#pragma once

#include <memory>

#include "benchmark_reporter_interface.h"
#include "html_document.h"

class HTMLBenchmarkReporter: public BenchmarkReporter
{
public:
    HTMLBenchmarkReporter( const char* fileName );

    virtual void WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results ) override;

    virtual void Flush() override;

    std::shared_ptr<HTMLDocument> GetHTMLDocument()
    {
        return document_;
    }
private:
    std::shared_ptr<HTMLDocument> document_;
    static const std::unordered_map<long double, std::string> avgDurationsUnits;
    static const std::unordered_map<long double, std::string> durationPerElementUnits;
    static const std::unordered_map<long double, std::string> elementsPerSecUnits;

    struct Units
    {
        long double avgDurationUnit = 0.0;
        boost::optional<long double> durationPerElementUnit;
        boost::optional<long double> elementsPerSecUnit;
    };

    std::vector<HTMLDocument::CellDescription> PrepareFirstRow( 
        const BenchmarkFixtureResultForFixture& results, const Units& units );
    std::vector<HTMLDocument::CellDescription> PrepareSecondRow(
        const BenchmarkFixtureResultForFixture& results );
    void AddOperationsResultsToRow(
        const BenchmarkFixtureResultForDevice& deviceData,
        const BenchmarkFixtureResultForFixture& allResults,
        const Units& units,
        std::vector<HTMLDocument::CellDescription>& row );
    std::unordered_map<OperationStep, OutputDurationType> CalcAverage(
        const BenchmarkFixtureResultForOperation& perOperationData,
        const std::vector<OperationStep>& steps );
    Units CalcUnits( const BenchmarkFixtureResultForFixture& results );
};
