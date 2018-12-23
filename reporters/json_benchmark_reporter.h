#pragma once

#include "reporters/benchmark_reporter.h"

#include <boost/property_tree/ptree.hpp>

class JsonBenchmarkReporter: public BenchmarkReporter
{
public:
    JsonBenchmarkReporter( const std::string& file_name );

    void AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results ) override;

    /*
        Optional method to flush all contents to output
    */
    void Flush() override;
private:
    std::string file_name_;
    bool pretty_ = true; // TODO make configurable?
    boost::property_tree::ptree tree_;
};
