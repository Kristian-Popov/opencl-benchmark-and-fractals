#include "reporters/json_benchmark_reporter.h"

#include <fstream>
#include <iomanip>

#include "fixtures/fixture_family.h"
#include "indicators/duration_indicator.h"
#include "indicators/element_processing_time_indicator.h"
#include "indicators/indicator_serializer.h"
#include "indicators/throughput_indicator.h"

#include <boost/log/trivial.hpp>

JsonBenchmarkReporter::JsonBenchmarkReporter( const std::string& file_name )
    : file_name_( file_name )
{
}

void JsonBenchmarkReporter::AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results )
{
    using nlohmann::json;
    std::vector<std::shared_ptr<IndicatorInterface>> indicators = {
        std::make_shared<DurationIndicator>( results )
    };
    if( results.fixture_family->element_count )
    {
        indicators.push_back( std::make_shared<ElementProcessingTimeIndicator>( results ) );
        indicators.push_back( std::make_shared<ThroughputIndicator>( results ) );
    }

    json indicator_list;
    for( const auto& indicator: indicators )
    {
        auto& d = IndicatorSerializer::Serialize( indicator );
        indicator_list[d.first] = d.second;
    }

    tree_[results.fixture_family->name] = indicator_list;
}

void JsonBenchmarkReporter::Flush()
{
    try
    {
        std::ofstream o( file_name_ );
        if( pretty_ )
        {
            o << std::setw( 4 );
        }
        o << tree_ << std::endl;
    }
    catch( std::exception& e )
    {
        BOOST_LOG_TRIVIAL( error ) << "Caught exception when building report in JSON format and writing to a file " << file_name_ << ": " << e.what();
        throw;
    }
}
