#include "fixture_runner.h"

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
#include "csv_document.h"
#include "sequential_values_iterator.h"
#include "random_values_iterator.h"

#include <boost/compute/compute.hpp>
#include <boost/math/constants/constants.hpp>

void FixtureRunner::Run( std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter, bool logProgress )
{
    // TODO add progress logging
    std::vector<std::shared_ptr<Fixture>> fixtures;
    const std::vector<int> dataSizesForTrivialFactorial = {100, 1000, 100000, 1000000, 100000000};
    std::transform( dataSizesForTrivialFactorial.begin(), dataSizesForTrivialFactorial.end(), std::back_inserter( fixtures ),
        []( int dataSize )
    {
        typedef std::uniform_int_distribution<int> Distribution;
        typedef RandomValuesIterator<int, Distribution> Iterator;
        return std::make_shared<TrivialFactorialFixture<Iterator>>(
            Iterator( Distribution( 0, 20 ) ),
            dataSize );
    } );
    cl_float frequency = 1.0f;
    const cl_float pi = boost::math::constants::pi<cl_float>();

    std::vector<DampedWaveFixtureParameters<cl_float>> params = 
        {DampedWaveFixtureParameters<cl_float>( 1000.0f, 0.001f, 2 * pi*frequency, 0.0f, 1.0f )};
    
    cl_float min = -10.0f;
    cl_float max = 10.0f;
    cl_float step = 0.001f;
    size_t dataSize = static_cast<size_t>( ( max - min ) / step ); // TODO something similar can be useful in Utils
    auto dampedWaveFixture = std::make_shared<DampedWaveFixture<cl_float, SequentialValuesIterator<cl_float>>>( params,
        SequentialValuesIterator<cl_float>( min, step ), dataSize );
    fixtures.push_back( dampedWaveFixture );

    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;
    auto targetFixtureExecutionTime = std::chrono::milliseconds( 10 ); // Time how long fixture should execute
    int minIterations = 10; // Minimal number of iterations

    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( std::shared_ptr<Fixture>& fixture : fixtures )
    {
        fixture->Initialize();
        BenchmarkTimeWriterInterface::BenchmarkFixtureResultForFixture dataForTimeWriter;
        dataForTimeWriter.operationSteps = fixture->GetSteps();
        dataForTimeWriter.fixtureName = fixture->Description();

        for( boost::compute::platform& platform : platforms )
        {
            BenchmarkTimeWriterInterface::BenchmarkFixtureResultForPlatform perPlatformResults;
            perPlatformResults.platformName = platform.name();

            std::vector<boost::compute::device> devices = platform.devices();
            for( boost::compute::device& device : devices )
            {
                BenchmarkTimeWriterInterface::BenchmarkFixtureResultForDevice perDeviceResults;
                perDeviceResults.deviceName = device.name();

                try
                {
                    // TODO create context for every device only once
                    boost::compute::context context( device );

                    std::vector<std::unordered_map<OperationStep, Fixture::Duration>> results;

                    // Warm-up for one iteration to get estimation of execution time
                    std::unordered_map<OperationStep, Fixture::Duration> warmupResult = fixture->Execute( context );
                    OutputDurationType totalOperationDuration = std::accumulate( warmupResult.begin(), warmupResult.end(), OutputDurationType::zero(),
                        []( OutputDurationType acc, const std::pair<OperationStep, Fixture::Duration>& r )
                    {
                        return acc + r.second;
                    } );
                    int iterationCount = static_cast<int>( std::ceil( targetFixtureExecutionTime / totalOperationDuration ) );
                    iterationCount = std::max( iterationCount, minIterations );
                    EXCEPTION_ASSERT( iterationCount >= 1 );

                    fixture->VerifyResults();

                    for( int i = 0; i < iterationCount; ++i )
                    {
                        results.push_back( fixture->Execute( context ) );
                    }

                    std::vector<std::unordered_map<OperationStep, BenchmarkTimeWriterInterface::OutputDurationType>> resultsForWriter;
                    for( const auto& res : results )
                    {
                        std::unordered_map<OperationStep, BenchmarkTimeWriterInterface::OutputDurationType> m;
                        for( const std::pair<OperationStep, Fixture::Duration>& p : res )
                        {
                            m.insert( std::make_pair( p.first,
                                std::chrono::duration_cast<BenchmarkTimeWriterInterface::OutputDurationType>( p.second ) ) );
                        }
                        resultsForWriter.push_back( m );
                    }
                    perDeviceResults.perOperationResults = resultsForWriter;
                }
                catch( boost::compute::opencl_error& e )
                {
                    std::cout << "OpenCL error occured: " << e.what() << std::endl;
                }
                catch( std::exception& e )
                {
                    std::cout << "Exception occured: " << e.what() << std::endl;
                }

                perPlatformResults.perDeviceResults.push_back( perDeviceResults );
            }

            dataForTimeWriter.perFixtureResults.push_back( perPlatformResults );
        }
        fixture->Finalize();
        timeWriter->WriteResultsForFixture( dataForTimeWriter );

        // Destroy fixture to release some memory sooner
        fixture.reset();
    }
    timeWriter->Flush();

    CSVDocument csvDocument( "damped_wave.csv" );
    csvDocument.AddValues( dampedWaveFixture->GetResults() );
    csvDocument.BuildAndWriteToDisk();
}