#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "trivial_factorial_fixture.h"
#include "damped_wave_fixture.h"
#include "operation_step.h"
#include "benchmark_fixture_html_builder.h"
#include "html_document.h"
#include "csv_document.h"

#include "boost/compute/compute.hpp"
#include <boost/math/constants/constants.hpp>

int main( int argc, char** argv )
{
    std::vector<std::shared_ptr<Fixture>> fixtures;
    const std::vector<int> dataSizesForTrivialFactorial = {100, 1000, 100000, 1000000, 100000000};
    std::transform( dataSizesForTrivialFactorial.begin(), dataSizesForTrivialFactorial.end(), std::back_inserter(fixtures),
        [] (int dataSize)
        {
            return std::make_shared<TrivialFactorialFixture>(dataSize);
        } );
    cl_float frequency = 1.0f;
    const cl_float pi = boost::math::constants::pi<cl_float>();

    std::vector<DampedWaveFixture<cl_float>::Parameters> params = { DampedWaveFixture<cl_float>::Parameters( 1000.0f, 0.001f, 2 * pi*frequency, 0.0f, 1.0f ) };
    auto dampedWaveFixture = std::make_shared<DampedWaveFixture<cl_float>>( params, 0.0f, 1.0f,
        DampedWaveFixture<cl_float>::DataPattern::Linear, 0.001f );
    fixtures.push_back( dampedWaveFixture );

    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;
    auto targetFixtureExecutionTime = std::chrono::milliseconds(10); // Time how long fixture should execute
    int minIterations = 10; // Minimal number of iterations
    const char* outputHTMLFileName = "output.html";
    BenchmarkFixtureHTMLBuilder htmlDocumentBuilder( outputHTMLFileName );

    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for (std::shared_ptr<Fixture>& fixture: fixtures)
    {
        fixture->Initialize();
        BenchmarkFixtureHTMLBuilder::BenchmarkFixtureResultForFixture dataForHTMLBuilder;
        dataForHTMLBuilder.operationSteps = fixture->GetSteps();
        dataForHTMLBuilder.fixtureName = fixture->Description();

        for( boost::compute::platform& platform: platforms )
        {
            BenchmarkFixtureHTMLBuilder::BenchmarkFixtureResultForPlatform perPlatformResults;
            perPlatformResults.platformName = platform.name();

            std::vector<boost::compute::device> devices = platform.devices();
            for( boost::compute::device& device: devices )
            {
                BenchmarkFixtureHTMLBuilder::BenchmarkFixtureResultForDevice perDeviceResults;
                perDeviceResults.deviceName = device.name();

                try
                {
                    std::cout << "\"" << platform.name() << "\", \"" << device.name() << "\", \"" << fixture->Description() << "\":" << std::endl;

                    // TODO create context for every device only once
                    boost::compute::context context( device );

                    std::vector<std::unordered_map<OperationStep, Fixture::ExecutionResult>> results;

                    // Warm-up for one iteration to get estimation of execution time
                    std::unordered_map<OperationStep, Fixture::ExecutionResult> warmupResult = fixture->Execute( context );
                    OutputDurationType totalOperationDuration = std::accumulate( warmupResult.begin(), warmupResult.end(), OutputDurationType::zero(),
                        [] ( OutputDurationType acc, const std::pair<OperationStep, Fixture::ExecutionResult>& r )
                        {
                            return acc + r.second.duration;
                        } );
                    int iterationCount = static_cast<int>(std::ceil(targetFixtureExecutionTime / totalOperationDuration));
                    iterationCount = std::max( iterationCount, minIterations );
                    EXCEPTION_ASSERT(iterationCount >= 1);

                    fixture->VerifyResults();

                    for( int i = 0; i < iterationCount; ++i)
                    {
                        results.push_back(fixture->Execute( context ) );
                    }

                    for( OperationStep id : fixture->GetSteps() )
                    {
                        std::vector<OutputDurationType> perOperationResults;
                        std::transform( results.begin(), results.end(), std::back_inserter( perOperationResults ),
                            [id]( const std::unordered_map<OperationStep, Fixture::ExecutionResult>& d )
                        {
                            const Fixture::ExecutionResult& r = d.at( id );
                            EXCEPTION_ASSERT( r.operationId == id );
                            return std::chrono::duration_cast<OutputDurationType>( r.duration );
                        } );

                        //TODO "accumulate" can safely be changed to "reduce" here to increase performance
                        OutputDurationType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), OutputDurationType::zero() ) / perOperationResults.size();

                        perDeviceResults.perOperationResults.insert( { id, avg } );
                    }

                    for ( OperationStep id : fixture->GetSteps() )
                    {
                        std::vector<double> perOperationResults;
                        std::transform(results.begin(), results.end(), std::back_inserter( perOperationResults ),
                            [id] (const std::unordered_map<OperationStep, Fixture::ExecutionResult>& d )
                            {
                                const Fixture::ExecutionResult& r = d.at(id);
                                EXCEPTION_ASSERT( r.operationId == id );
                                return std::chrono::duration_cast<OutputDurationType>(r.duration).count();
                            } );

                        auto minmax = std::minmax_element( perOperationResults.begin(), perOperationResults.end() );
                        OutputNumericType min = *minmax.first;
                        OutputNumericType max = *minmax.second;

                        //TODO "accumulate" can safely be changed to "reduce" here to increase performance
                        OutputNumericType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), 0.0) / perOperationResults.size();

                        std::cout << "\t\"" << OperationStepDescriptionRepository::Get(id) << "\": " << 
                            min << " - " << max << " (avg " << avg << ") microseconds" << std::endl;
                    }

                    std::cout << std::endl;
                }
                catch(boost::compute::opencl_error& e)
                {
                    std::cout << "OpenCL error occured: " << e.what() << std::endl;
                }
                catch(std::exception& e)
                {
                    std::cout << "Exception occured: " << e.what() << std::endl;
                }

                perPlatformResults.perDeviceResults.push_back(perDeviceResults);
            }

            dataForHTMLBuilder.perFixtureResults.push_back(perPlatformResults);
        }
        fixture->Finalize();
        htmlDocumentBuilder.AddFixtureResults( dataForHTMLBuilder );

        // Destroy fixture to release some memory sooner
        fixture.reset();
    }
    htmlDocumentBuilder.GetHTMLDocument()->BuildAndWriteToDisk();

    CSVDocument csvDocument("damped_wave.csv");
    csvDocument.AddValues( dampedWaveFixture->GetResults() );
    csvDocument.BuildAndWriteToDisk();

    return 0;
}

