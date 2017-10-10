#include "stdout_benchmark_time_writer.h"

#include "utils.h"

#include <boost/format.hpp>

void StdoutBenchmarkTimeWriter::WriteResultsForFixture( const BenchmarkFixtureResultForFixture& results )
{
    std::vector<OperationStep> operationSteps = results.operationSteps;
    for (const auto& perFixtureResults: results.perFixtureResults )
    {
        for( const auto& perDeviceResults : perFixtureResults.perDeviceResults )
        {
            std::unordered_map<OperationStep, OutputDurationType> avg = CalcAverage( perDeviceResults.perOperationResults, operationSteps );
            std::unordered_map<OperationStep, std::pair<OutputDurationType, OutputDurationType>> minmax = CalcMinMax( perDeviceResults.perOperationResults, operationSteps );

            std::string text = ( boost::format( "\"%1%\", \"%2%\", \"%3%\":" ) % 
                results.fixtureName % 
                perFixtureResults.platformName %
                perDeviceResults.deviceName ).str();
            std::cout << text << std::endl;
            for (OperationStep step: results.operationSteps )
            {
                OutputDurationType currentAvg = avg.at( step );
                OutputDurationType currentMin = minmax.at( step ).first;
                OutputDurationType currentMax = minmax.at( step ).second;

                text = ( boost::format( "\t\"%1%\": %2% - %3% (avg %4%) microseconds" ) %
                    OperationStepDescriptionRepository::Get( step ) %
                    currentMin.count() %
                    currentMax.count() %
                    currentAvg.count() ).str();
                std::cout << text << std::endl;
            }
        }
    }
}

std::unordered_map<OperationStep, BenchmarkTimeWriterInterface::OutputDurationType> StdoutBenchmarkTimeWriter::CalcAverage(
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

std::unordered_map<OperationStep, std::pair<BenchmarkTimeWriterInterface::OutputDurationType, BenchmarkTimeWriterInterface::OutputDurationType>> StdoutBenchmarkTimeWriter::CalcMinMax(
    const BenchmarkTimeWriterInterface::BenchmarkFixtureResultForOperation& perOperationData,
    const std::vector<OperationStep>& steps )
{
    std::unordered_map<OperationStep, std::pair<OutputDurationType, OutputDurationType>> perOperationMinMax;
    for( OperationStep id : steps )
    {
        std::vector<OutputDurationType> perOperationResults;
        EXCEPTION_ASSERT( !perOperationData.empty() );
        std::transform( perOperationData.cbegin(), perOperationData.cend(), std::back_inserter( perOperationResults ),
            [id]( const std::unordered_map<OperationStep, OutputDurationType>& d )
        {
            return std::chrono::duration_cast<OutputDurationType>( d.at( id ) );
        } );

        auto minmax = std::minmax_element( perOperationResults.begin(), perOperationResults.end() );

        perOperationMinMax.insert( { id, std::make_pair( *minmax.first, *minmax.second ) } );
    }
    return perOperationMinMax;
}