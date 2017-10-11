#pragma once

#include <memory>

#include "benchmark_time_writer_interface.h"

class FixtureRunner
{
public:
    static void Run( std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter, bool logProgress );
};