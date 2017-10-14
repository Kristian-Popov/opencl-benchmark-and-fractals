#include "html_benchmark_time_writer.h"

#include <unordered_map>
#include <algorithm>
#include <iterator>

#include "utils.h"

HTMLBenchmarkTimeWriter::HTMLBenchmarkTimeWriter( const char* fileName )
    : document_( std::make_shared<HTMLDocument>( fileName ) )
{}

void HTMLBenchmarkTimeWriter::AddOperationsResultsToRow( 
    const BenchmarkFixtureResultForDevice& deviceData,
    const std::vector<OperationStep>& steps,
    std::vector<HTMLDocument::CellDescription>& row )
{
    if( deviceData.perOperationResults.empty() )
    {
        // If results are absent for particular device, add a wide cell with a failure description
        row.push_back( HTMLDocument::CellDescription( 
            deviceData.failureReason.get_value_or( std::string() ),
            false, 1, steps.size() ) );
    }
    else
    {
        auto avgDurations = CalcAverage( deviceData.perOperationResults, steps );
        for( OperationStep step : steps )
        {
            row.push_back( HTMLDocument::CellDescription( std::to_string( avgDurations.at( step ).count() ) ) );
        }
    }
}

void HTMLBenchmarkTimeWriter::WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results )
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
            AddOperationsResultsToRow(firstDeviceResults, results.operationSteps, row);
            rows.push_back( row );
        }

        for ( int deviceNum = 1; deviceNum < perPlatformResults.perDeviceResults.size(); deviceNum++ )
        {
            const auto& perDeviceResult = perPlatformResults.perDeviceResults.at( deviceNum );
            std::vector<HTMLDocument::CellDescription> row = {
                HTMLDocument::CellDescription( perDeviceResult.deviceName ),
            };
            AddOperationsResultsToRow( perDeviceResult, results.operationSteps, row );
            rows.push_back( row );
        }
    }

    document_->AddTable(rows);
}

std::unordered_map<OperationStep, BenchmarkTimeWriterInterface::OutputDurationType> HTMLBenchmarkTimeWriter::CalcAverage(
    const BenchmarkTimeWriterInterface::BenchmarkFixtureResultForOperation& perOperationData,
    const std::vector<OperationStep>& steps )
{
    std::unordered_map<OperationStep, BenchmarkTimeWriterInterface::OutputDurationType> perOperationAvg;
    for( OperationStep id : steps )
    {
        std::vector<BenchmarkTimeWriterInterface::OutputDurationType> perOperationResults;
        EXCEPTION_ASSERT( !perOperationData.empty() );
        std::transform( perOperationData.cbegin(), perOperationData.cend(), std::back_inserter( perOperationResults ),
            [id]( const std::unordered_map<OperationStep, OutputDurationType>& d )
        {
            return std::chrono::duration_cast<OutputDurationType>( d.at( id ) );
        } );

        //TODO "accumulate" can safely be changed to "reduce" here to increase performance
        OutputDurationType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), OutputDurationType::zero() ) / perOperationResults.size();

        perOperationAvg.insert( {id, avg} );
    }
    return perOperationAvg;
}

void HTMLBenchmarkTimeWriter::Flush()
{
    document_->BuildAndWriteToDisk();
}