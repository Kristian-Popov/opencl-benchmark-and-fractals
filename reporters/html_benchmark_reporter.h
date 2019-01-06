#pragma once

#include <memory>

#include "reporters/benchmark_reporter.h"

class HtmlDocument;

class HtmlBenchmarkReporter: public BenchmarkReporter
{
public:
    HtmlBenchmarkReporter( const std::string& file_name );

    virtual void AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results ) override;

    virtual void Flush() override;

    std::shared_ptr<HtmlDocument> GetHTMLDocument()
    {
        return document_;
    }
private:
    std::shared_ptr<HtmlDocument> document_;
    static const std::unordered_map<long double, std::string> avgDurationsUnits;
    static const std::unordered_map<long double, std::string> durationPerElementUnits;
    static const std::unordered_map<long double, std::string> elementsPerSecUnits;
};
