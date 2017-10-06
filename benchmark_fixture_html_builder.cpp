#include "benchmark_fixture_html_builder.h"

#include <unordered_map>
#include <algorithm>
#include <iterator>


#include "utils.h"

namespace
{
    const std::unordered_map<OperationId, std::string> operationDescriptions = {
        std::make_pair( OperationId::CopyInputDataToDevice, "Copy input data to device" ),
        std::make_pair( OperationId::Calculate, "Calculation" ),
        std::make_pair( OperationId::CopyOutputDataFromDevice, "Copy output data from device" ),
    };
}

BenchmarkFixtureHTMLBuilder::BenchmarkFixtureHTMLBuilder( const char* fileName )
    : document_( std::make_shared<HTMLDocument>( fileName ) )
{}

void BenchmarkFixtureHTMLBuilder::AddFixtureResults( const BenchmarkFixtureResultForFixture& results )
{
    document_->AddHeader( results.fixtureName );

    std::vector<std::vector<HTMLDocument::CellDescription>> rows;
    typedef HTMLDocument::CellDescription CellDescription;
    rows.push_back( { 
        CellDescription("Platform name", true, 2), 
        CellDescription( "Device name", true, 2 ), 
        CellDescription( "Result, us", true, 1, static_cast<int>(operationDescriptions.size()) )
    } );

    std::vector<HTMLDocument::CellDescription> row;
    // TODO make sure that headers and results are in correct order
    std::transform( operationDescriptions.cbegin(), operationDescriptions.cend(), std::back_inserter( row ),
        [] (const std::pair<OperationId, std::string>& v)
        {
            return CellDescription( v.second );
        } );
    rows.push_back( row );
    for (const BenchmarkFixtureResultForPlatform& perPlatformResults: results.perFixtureResults)
    {
        // First row is a bit special since platform name should also be there
        EXCEPTION_ASSERT( perPlatformResults.perDeviceResults.size() >= 1 );
        {
            const BenchmarkFixtureResultForDevice& firstDeviceResults = perPlatformResults.perDeviceResults.at( 0 );
            std::vector<HTMLDocument::CellDescription> row = {
                HTMLDocument::CellDescription( perPlatformResults.platformName, false, static_cast<int>(perPlatformResults.perDeviceResults.size()) ),
                HTMLDocument::CellDescription( firstDeviceResults.deviceName ),
            };
            // TODO make sure that we're writing data in a right order
            std::transform( firstDeviceResults.perOperationResults.cbegin(), firstDeviceResults.perOperationResults.cend(), std::back_inserter( row ),
                []( const std::pair<OperationId, OutputDurationType>& perOperationResult )
                {
                    return HTMLDocument::CellDescription( std::to_string( perOperationResult.second.count() ) );
                } );
            rows.push_back( row );
        }

        for ( int deviceNum = 1; deviceNum < perPlatformResults.perDeviceResults.size(); deviceNum++ )
        {
            const auto& perDeviceResult = perPlatformResults.perDeviceResults.at( deviceNum );
            std::vector<HTMLDocument::CellDescription> row = {
                HTMLDocument::CellDescription( perDeviceResult.deviceName ),
            };
            
            // TODO make sure that we're writing data in a right order
            std::transform( perDeviceResult.perOperationResults.cbegin(), perDeviceResult.perOperationResults.cend(), std::back_inserter( row ),
                []( const std::pair<OperationId, OutputDurationType>& perOperationResult )
                {
                    return HTMLDocument::CellDescription( std::to_string( perOperationResult.second.count() ) );
                } );
            rows.push_back( row );
        }
    }

    document_->AddTable(rows);
}