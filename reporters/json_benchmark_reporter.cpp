#include "reporters/json_benchmark_reporter.h"

#include "fixtures/fixture_family.h"
#include "indicators/duration_indicator.h"
#include "indicators/element_processing_time_indicator.h"
#include "indicators/indicator_serializer.h"
#include "indicators/throughput_indicator.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/log/trivial.hpp>

JsonBenchmarkReporter::JsonBenchmarkReporter( const std::string& file_name )
    : file_name_( file_name )
{
}

void JsonBenchmarkReporter::AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results )
{
    std::vector<std::shared_ptr<IndicatorInterface>> indicators = {
        std::make_shared<DurationIndicator>( results )
    };
    if( results.fixture_family->element_count )
    {
        indicators.push_back( std::make_shared<ElementProcessingTimeIndicator>( results ) );
        indicators.push_back( std::make_shared<ThroughputIndicator>( results ) );
    }

	boost::property_tree::ptree indicator_list;
    for( const auto& indicator: indicators )
    {
        auto& d = IndicatorSerializer::Serialize( indicator );
        indicator_list.add_child( d.first, d.second );
    }

	tree_.put_child( results.fixture_family->name, indicator_list );
}

void JsonBenchmarkReporter::Flush()
{
    try
    {
        // TODO force saving in UTF-8 for compliance with RFC 8259 ?
        boost::property_tree::json_parser::write_json( file_name_, tree_, std::locale(), pretty_ );
    }
    catch( std::exception& e )
    {
        BOOST_LOG_TRIVIAL( error ) << "Caught exception when building report in JSON format and writing to a file " << file_name_ << ": " << e.what();
        throw;
    }
}
