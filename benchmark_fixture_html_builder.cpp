#include "benchmark_fixture_html_builder.h"

#include <unordered_map>
#include <algorithm>
#include <iterator>

#include "utils.h"

BenchmarkFixtureHTMLBuilder::BenchmarkFixtureHTMLBuilder( const char* fileName )
    : document_( std::make_shared<HTMLDocument>( fileName ) )
{}

void BenchmarkFixtureHTMLBuilder::AddFixtureResults( const BenchmarkFixtureResultForFixture& results )
{
    EXCEPTION_ASSERT( !results.operationSteps.empty() );
    document_->AddHeader( results.fixtureName );

    std::vector<std::vector<HTMLDocument::CellDescription>> rows;
    typedef HTMLDocument::CellDescription CellDescription;
    rows.push_back( { 
        CellDescription( "Platform name", true, 2 ), 
        CellDescription( "Device name", true, 2 ), 
        CellDescription( "Result, us", true, 1, static_cast<int>(results.operationSteps.size()) )
    } );

    {
        std::vector<HTMLDocument::CellDescription> row;
        std::transform( results.operationSteps.cbegin(), results.operationSteps.cend(), std::back_inserter( row ),
            []( OperationStep step )
            {
                return CellDescription( OperationStepDescriptionRepository::Get( step ) );
            } );
        rows.push_back( row );
    }
    
    for (const BenchmarkFixtureResultForPlatform& perPlatformResults: results.perFixtureResults)
    {
        // First row is a bit special since platform name should also be there
        EXCEPTION_ASSERT( !perPlatformResults.perDeviceResults.empty() );
        {
            const BenchmarkFixtureResultForDevice& firstDeviceResults = perPlatformResults.perDeviceResults.at( 0 );
            std::vector<HTMLDocument::CellDescription> row = {
                HTMLDocument::CellDescription( perPlatformResults.platformName, false, static_cast<int>(perPlatformResults.perDeviceResults.size()) ),
                HTMLDocument::CellDescription( firstDeviceResults.deviceName ),
            };
            std::transform( firstDeviceResults.perOperationResults.cbegin(), firstDeviceResults.perOperationResults.cend(), std::back_inserter( row ),
                []( const std::pair<OperationStep, OutputDurationType>& perOperationResult )
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
                []( const std::pair<OperationStep, OutputDurationType>& perOperationResult )
                {
                    return HTMLDocument::CellDescription( std::to_string( perOperationResult.second.count() ) );
                } );
            rows.push_back( row );
        }
    }

    document_->AddTable(rows);
}