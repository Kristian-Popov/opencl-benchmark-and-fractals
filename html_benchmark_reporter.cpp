#include "html_benchmark_reporter.h"

#include <unordered_map>
#include <algorithm>
#include <iterator>

#include "utils.h"

const std::unordered_map<long double, std::string> HTMLBenchmarkReporter::avgDurationsUnits =
{
    { 1e-9, "nanoseconds" },
    { 1e-6, "microseconds" },
    { 1e-3, "milliseconds" },
    { 1, "seconds" },
    { 60, "minutes" },
    { 3600, "hours" }
};

const std::unordered_map<long double, std::string> HTMLBenchmarkReporter::durationPerElementUnits =
{
    {1e-9, "nanoseconds/elem."},
    {1e-6, "microseconds/elem."},
    {1e-3, "milliseconds/elem."},
    {1, "seconds/elem."},
    {60, "minutes/elem."},
    {3600, "hours/elem."}
};

const std::unordered_map<long double, std::string> HTMLBenchmarkReporter::elementsPerSecUnits =
{
    { 1, "elem./second" },
    { 1e+3, "thousands elem./second"},
    { 1e+6, "millions elem./second"},
    { 1e+9, "billions elem./second"}
};

HTMLBenchmarkReporter::HTMLBenchmarkReporter( const char* fileName )
    : document_( std::make_shared<HTMLDocument>( fileName ) )
{}

void HTMLBenchmarkReporter::AddOperationsResultsToRow( 
    const BenchmarkFixtureResultForDevice& deviceData,
    const BenchmarkResultForFixture& allResults,
    const HTMLBenchmarkReporter::Units& units,
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
            long double dur = avgDurations.at( step ).count() / units.avgDurationUnit;
            row.push_back( HTMLDocument::CellDescription( std::to_string( dur ) ) );
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
            EXCEPTION_ASSERT( units.durationPerElementUnit && units.elementsPerSecUnit );
            for( OperationStep step : steps )
            {
                long double timePerElement = avgDurations.at( step ).count() / allResults.elementsCount.get();
                timePerElement /= units.durationPerElementUnit.get();
                row.push_back( HTMLDocument::CellDescription( std::to_string( timePerElement ) ) );
            }
            for( OperationStep step : steps )
            {
                typedef std::chrono::duration<double> DurationInSec;
                long double elementsPerSec = allResults.elementsCount.get() / 
                    std::chrono::duration_cast<DurationInSec>( avgDurations.at( step ) ).count();
                elementsPerSec /= units.elementsPerSecUnit.get();
                row.push_back( HTMLDocument::CellDescription( std::to_string( elementsPerSec ) ) );
            }
        }
    }
}

std::vector<HTMLDocument::CellDescription> HTMLBenchmarkReporter::PrepareFirstRow(
    const BenchmarkFixtureResultForFixture& results,
    const HTMLBenchmarkReporter::Units& units )
{
    std::string avgDurationTitle = "Duration, " + avgDurationsUnits.at( units.avgDurationUnit );
    typedef HTMLDocument::CellDescription CellDescription;
    std::vector<HTMLDocument::CellDescription> result = {
        CellDescription( "Platform name", true, 2 ),
        CellDescription( "Device name", true, 2 ),
        CellDescription( avgDurationTitle, true, 1, static_cast<int>( results.operationSteps.size() ) )
    };
    if (results.elementsCount)
    {
        EXCEPTION_ASSERT( units.durationPerElementUnit && units.elementsPerSecUnit );

        std::string durationPerElementTitle = "Duration per element, " +
            durationPerElementUnits.at( units.durationPerElementUnit.get() );
        result.push_back( CellDescription( durationPerElementTitle, true, 1,
            static_cast<int>( results.operationSteps.size() ) ) );

        std::string elemPerSecTitle = "Elements per second, " +
            elementsPerSecUnits.at( units.elementsPerSecUnit.get() );
        result.push_back( CellDescription( elemPerSecTitle, true, 1,
            static_cast<int>( results.operationSteps.size() ) ) );
    }
    return result;
}

std::vector<HTMLDocument::CellDescription> HTMLBenchmarkReporter::PrepareSecondRow(
    const BenchmarkFixtureResultForFixture& results )
{
    // This row contains only names of the operation steps. If we don't have elements count,
    // we should have only one set of these names. Otherwise we need three of these sets.
    typedef HTMLDocument::CellDescription CellDescription;
    std::vector<HTMLDocument::CellDescription> row;
    // TODO something like that would be great (do not add extra sessions of elementsCount is given but equal to 1)
    //int repetitionsCount = ( results.elementsCount && results.elementsCount.get() > 1 ) ? 3 : 1;
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

void HTMLBenchmarkReporter::WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results )
{
    EXCEPTION_ASSERT( !results.operationSteps.empty() );
    document_->AddHeader( results.fixtureName );

    std::vector<std::vector<HTMLDocument::CellDescription>> rows;
    typedef HTMLDocument::CellDescription CellDescription;
    Units units = CalcUnits( results );
    rows.push_back( PrepareFirstRow( results, units ) );
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
            AddOperationsResultsToRow(firstDeviceResults, results, units, row);
            rows.push_back( row );
        }

        for ( int deviceNum = 1; deviceNum < perPlatformResults.perDeviceResults.size(); deviceNum++ )
        {
            const auto& perDeviceResult = perPlatformResults.perDeviceResults.at( deviceNum );
            std::vector<HTMLDocument::CellDescription> row = {
                HTMLDocument::CellDescription( perDeviceResult.deviceName ),
            };
            AddOperationsResultsToRow( perDeviceResult, results, units, row );
            rows.push_back( row );
        }
    }

    document_->AddTable(rows);
}

std::unordered_map<OperationStep, BenchmarkReporter::OutputDurationType> HTMLBenchmarkReporter::CalcAverage(
    const BenchmarkReporter::BenchmarkFixtureResultForOperation& perOperationData,
    const std::vector<OperationStep>& steps )
{
    std::unordered_map<OperationStep, BenchmarkReporter::OutputDurationType> perOperationAvg;
    for( OperationStep id : steps )
    {
        std::vector<BenchmarkReporter::OutputDurationType> perOperationResults;
        if( !perOperationData.empty() )
        {
            std::transform( perOperationData.cbegin(), perOperationData.cend(), std::back_inserter( perOperationResults ),
                [id]( const std::unordered_map<OperationStep, OutputDurationType>& d )
            {
                // If duration for the given step is not given, use quiet NaN instead
                return std::chrono::duration_cast<OutputDurationType>( Utils::FindInMapWithDefault( d, id, 
                    OutputDurationType( std::numeric_limits<OutputNumericType>::quiet_NaN() ) 
                ) );
            } );

            //TODO "accumulate" can safely be changed to "reduce" here to increase performance
            OutputDurationType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), OutputDurationType::zero() ) / perOperationResults.size();

            perOperationAvg.insert( {id, avg} );
        }
    }
    return perOperationAvg;
}

HTMLBenchmarkReporter::Units HTMLBenchmarkReporter::CalcUnits( const BenchmarkFixtureResultForFixture& results )
{
    Units result;
    std::vector<long double> avgDurationsInSec;
    for( const BenchmarkFixtureResultForPlatform& perPlatformResults : results.perFixtureResults )
    {
        for ( const BenchmarkFixtureResultForDevice& perDeviceResults: perPlatformResults.perDeviceResults)
        {
            std::unordered_map<OperationStep, OutputDurationType> avgDurations =
                CalcAverage( perDeviceResults.perOperationResults, results.operationSteps );
            // We don't care about step order here, so we can copy durations directly from a map
            std::transform( avgDurations.begin(), avgDurations.end(), std::back_inserter( avgDurationsInSec ),
                [] (const std::pair<OperationStep, OutputDurationType>& p)
                {
                    typedef std::chrono::duration<long double> LongDoubleDurationInSec;
                    return std::chrono::duration_cast<LongDoubleDurationInSec>( p.second ).count();
                } );
        }
    }
    // Something went horribly wrong, return some default value if we have no duration data
    if ( avgDurationsInSec.empty() )
    {
        result.avgDurationUnit = avgDurationsUnits.cbegin()->first;
        if( results.elementsCount )
        {
            result.durationPerElementUnit = durationPerElementUnits.cbegin()->first;
            result.elementsPerSecUnit = elementsPerSecUnits.cbegin()->first;
        }
        return result;
    }
    {
        std::vector<long double> avgDurationsList;
        std::transform( avgDurationsUnits.cbegin(), avgDurationsUnits.cend(), std::back_inserter( avgDurationsList ),
            Utils::SelectFirst<long double, std::string> );
        result.avgDurationUnit = Utils::ChooseConvenientUnit( avgDurationsInSec, avgDurationsList );
    }

    if (results.elementsCount)
    {
        size_t elementsCount = results.elementsCount.get();

        {
            std::vector<long double> durationsPerElementInSec;
            std::transform( avgDurationsInSec.begin(), avgDurationsInSec.end(), std::back_inserter( durationsPerElementInSec ),
                [elementsCount]( long double durationInSec )
            {
                return durationInSec / elementsCount;
            } );

            std::vector<long double> durationsPerElementUnitList;
            std::transform( durationPerElementUnits.cbegin(), durationPerElementUnits.cend(), 
                std::back_inserter( durationsPerElementUnitList ),
                Utils::SelectFirst<long double, std::string> );
            result.durationPerElementUnit = Utils::ChooseConvenientUnit( durationsPerElementInSec, durationsPerElementUnitList );
        }

        {
            std::vector<long double> elementsPerSec;
            std::transform( avgDurationsInSec.begin(), avgDurationsInSec.end(), std::back_inserter( elementsPerSec ),
                [elementsCount]( long double durationInSec )
            {
                return elementsCount / durationInSec;
            } );

            std::vector<long double> elementsPerSecUnitList;
            std::transform( elementsPerSecUnits.cbegin(), elementsPerSecUnits.cend(),
                std::back_inserter( elementsPerSecUnitList ),
                Utils::SelectFirst<long double, std::string> );
            result.elementsPerSecUnit = Utils::ChooseConvenientUnit( elementsPerSec, elementsPerSecUnitList );
        }
    }
    return result;
}

void HTMLBenchmarkReporter::Flush()
{
    document_->BuildAndWriteToDisk();
}
