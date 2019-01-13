#include "reporters/json_benchmark_reporter.h"

#include <fstream>
#include <iomanip>
#include <time.h>

#include "fixtures/fixture_family.h"
#include "indicators/duration_indicator.h"
#include "indicators/element_processing_time_indicator.h"
#include "indicators/failure_indicator.h"
#include "indicators/indicator_serializer.h"
#include "indicators/throughput_indicator.h"

#include <boost/log/trivial.hpp>

namespace
{
    std::string GetCurrentTimeString()
    {
        // TODO replace with some library?
        // Based on https://stackoverflow.com/a/10467633
        time_t now = time( 0 );
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime( &now );
        strftime( buf, sizeof( buf ), "%Y-%m-%d.%X", &tstruct );
        return buf;
    }
}

JsonBenchmarkReporter::JsonBenchmarkReporter( const std::string& file_name )
    : file_name_( file_name )
{
}

void JsonBenchmarkReporter::Initialize( const PlatformList& platform_list )
{
    tree_["base_info"] = {
        {"about", "This file was built by OpenCL benchmark."},
        {"time", GetCurrentTimeString()}
    };
    for( auto& platform : platform_list.AllPlatforms() )
    {
        nlohmann::json devices = nlohmann::json::array();
        for( auto& device : platform->GetDevices() )
        {
            devices.push_back( device->UniqueName() );
        }
        tree_["device_list"][platform->Name()] = devices;
    }
}

void JsonBenchmarkReporter::AddFixtureFamilyResults( const BenchmarkResultForFixtureFamily& results )
{
    using nlohmann::json;
    std::vector<std::shared_ptr<IndicatorInterface>> indicators = {
        std::make_shared<DurationIndicator>( results ),
        std::make_shared<FailureIndicator>( results ),
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

    tree_["results"][results.fixture_family->name] = indicator_list;
}

void JsonBenchmarkReporter::Flush()
{
    try
    {
        std::ofstream o( file_name_ );
        o.exceptions( std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit );
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
