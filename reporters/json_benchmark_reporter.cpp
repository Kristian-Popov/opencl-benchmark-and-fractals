#pragma once

#include "fixtures/fixture_family.h"

#include "reporters/json_benchmark_reporter.h"

#include "indicators/indicator_serializer.h"
#include "indicators/duration_indicator.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/log/trivial.hpp>

JsonBenchmarkReporter::JsonBenchmarkReporter( const std::string& file_name )
    : file_name_( file_name )
{
}

void JsonBenchmarkReporter::AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results )
{
	auto& duration_indicator = std::make_shared<DurationIndicator>( results );
    std::pair<std::string, boost::property_tree::ptree> d = IndicatorSerializer::Serialize( duration_indicator );
	boost::property_tree::ptree indicator_list;
	indicator_list.add_child( d.first, d.second );
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
