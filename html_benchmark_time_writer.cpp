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
    const BenchmarkFixtureResultForFixture& allResults,
    std::vector<HTMLDocument::CellDescription>& row )
{
    const std::vector<OperationStep>& steps = allResults.operationSteps;
    std::unordered_map<OperationStep, OutputDurationType> avgDurations = 
        CalcAverage( deviceData.perOperationResults, steps );
    if( deviceData.perOperationResults.empty() )
    {
        // If results are absent for particular device, add a wide cell with a failure description
        row.push_back( HTMLDocument::CellDescription( 
            deviceData.failureReason.get_value_or( std::string() ),
            false, 1, steps.size() ) );
    }
    else
    {
        for( OperationStep step : steps )
        {
            row.push_back( HTMLDocument::CellDescription( std::to_string( avgDurations.at( step ).count() ) ) );
        }
    }

    if (allResults.elementsCount)
    {
        if( deviceData.perOperationResults.empty() )
        {
            // If results are absent for particular device, add a wide cell with a failure description
            for (int i = 0; i < 2; ++i)
            {
                row.push_back( HTMLDocument::CellDescription(
                    deviceData.failureReason.get_value_or( std::string() ),
                    false, 1, steps.size() ) );
            }
        }
        else
        {
            for( OperationStep step : steps )
            {
                double timePerElementInUs = avgDurations.at( step ).count() / allResults.elementsCount.get();
                row.push_back( HTMLDocument::CellDescription( std::to_string( timePerElementInUs ) ) );
            }
            for( OperationStep step : steps )
            {
                typedef std::chrono::duration<double> DurationInSec;
                double elementsPerSec = allResults.elementsCount.get() / 
                    std::chrono::duration_cast<DurationInSec>( avgDurations.at( step ) ).count();
                row.push_back( HTMLDocument::CellDescription( std::to_string( elementsPerSec ) ) );
            }
        }
    }
}

std::vector<HTMLDocument::CellDescription> HTMLBenchmarkTimeWriter::PrepareFirstRow(
    const BenchmarkFixtureResultForFixture& results )
{
    typedef HTMLDocument::CellDescription CellDescription;
    std::vector<HTMLDocument::CellDescription> result = {
        CellDescription( "Platform name", true, 2 ),
        CellDescription( "Device name", true, 2 ),
        CellDescription( "Result, us", true, 1, static_cast<int>( results.operationSteps.size() ) )
    };
    if (results.elementsCount)
    {
        result.push_back( CellDescription( "Time per element, us/elem.", true, 1, 
            static_cast<int>( results.operationSteps.size() ) ) );
        result.push_back( CellDescription( "Elements per second, elem./s.", true, 1,
            static_cast<int>( results.operationSteps.size() ) ) );
    }
    return result;
}

std::vector<HTMLDocument::CellDescription> HTMLBenchmarkTimeWriter::PrepareSecondRow(
    const BenchmarkFixtureResultForFixture& results )
{
    // This row contains only names of the operation steps. Of we don't have elements count,
    // we should have only one set of these names. Otherwise we need three of these sets.
    typedef HTMLDocument::CellDescription CellDescription;
    std::vector<HTMLDocument::CellDescription> row;
    int repetitionsCount = results.elementsCount ? 3 : 1;
    for (int i = 0; i < repetitionsCount; ++i)
    {
        std::transform( results.operationSteps.cbegin(), results.operationSteps.cend(), std::back_inserter( row ),
            []( OperationStep step )
        {
            return CellDescription( OperationStepDescriptionRepository::Get( step ) );
        } );
    }
    return row;
}

void HTMLBenchmarkTimeWriter::WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results )
{
    EXCEPTION_ASSERT( !results.operationSteps.empty() );
    document_->AddHeader( results.fixtureName );

    std::vector<std::vector<HTMLDocument::CellDescription>> rows;
    typedef HTMLDocument::CellDescription CellDescription;
    rows.push_back( PrepareFirstRow( results ) );
    rows.push_back( PrepareSecondRow( results ) );
    
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
            AddOperationsResultsToRow(firstDeviceResults, results, row);
            rows.push_back( row );
        }

        for ( int deviceNum = 1; deviceNum < perPlatformResults.perDeviceResults.size(); deviceNum++ )
        {
            const auto& perDeviceResult = perPlatformResults.perDeviceResults.at( deviceNum );
            std::vector<HTMLDocument::CellDescription> row = {
                HTMLDocument::CellDescription( perDeviceResult.deviceName ),
            };
            AddOperationsResultsToRow( perDeviceResult, results, row );
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
        if( !perOperationData.empty() )
        {
            std::transform( perOperationData.cbegin(), perOperationData.cend(), std::back_inserter( perOperationResults ),
                [id]( const std::unordered_map<OperationStep, OutputDurationType>& d )
            {
                return std::chrono::duration_cast<OutputDurationType>( d.at( id ) );
            } );

            //TODO "accumulate" can safely be changed to "reduce" here to increase performance
            OutputDurationType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), OutputDurationType::zero() ) / perOperationResults.size();

            perOperationAvg.insert( {id, avg} );
        }
    }
    return perOperationAvg;
}

void HTMLBenchmarkTimeWriter::Flush()
{
    document_->BuildAndWriteToDisk();
}