#include "fixture_runner.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <random>

#include "data_verification_failed_exception.h"
#include "trivial_factorial_fixture.h"
#include "damped_wave_fixture.h"
#include "operation_step.h"
#include "csv_document.h"
#include "sequential_values_iterator.h"
#include "random_values_iterator.h"

#include <boost/log/trivial.hpp>
#include <boost/compute/compute.hpp>
#include <boost/math/constants/constants.hpp>

void FixtureRunner::CreateTrivialFixtures( std::vector<std::shared_ptr<Fixture>>& fixtures )
{
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
}

void FixtureRunner::CreateDampedWave2DFixtures(
    std::vector<std::shared_ptr<Fixture>>& fixtures,
    std::vector<std::shared_ptr<FixtureThatReturnsData<cl_float>>>& fixturesWithData )
{
    cl_float frequency = 1.0f;
    const cl_float pi = boost::math::constants::pi<cl_float>();

    cl_float min = -10.0f;
    cl_float max = 10.0f;
    cl_float step = 0.001f;
    size_t dataSize = static_cast<size_t>( ( max - min ) / step ); // TODO something similar can be useful in Utils
    {
        std::vector<DampedWaveFixtureParameters<cl_float>> params =
        {DampedWaveFixtureParameters<cl_float>( 1000.0f, 0.1f, 2 * pi*frequency, 0.0f, 1.0f )};
        auto dampedWaveFixture = std::make_shared<DampedWaveFixture<cl_float, SequentialValuesIterator<cl_float>>>( params,
            SequentialValuesIterator<cl_float>( min, step ), dataSize, "sequential input values" );
        fixtures.push_back( dampedWaveFixture );
        fixturesWithData.push_back( dampedWaveFixture );
    }
    {
        std::vector<DampedWaveFixtureParameters<cl_double>> params =
        {DampedWaveFixtureParameters<cl_double>( 1000.0, 0.1, 2 * pi*frequency, 0.0, 1.0 )};

        typedef std::normal_distribution<cl_double> Distribution;
        typedef RandomValuesIterator<cl_double, Distribution> Iterator;
        auto dampedWaveFixture = std::make_shared<DampedWaveFixture<cl_double, Iterator>>( params,
            Distribution( 0.0, 100.0 ), dataSize, "random input values" );
        fixtures.push_back( dampedWaveFixture );
    }

    {
        const size_t paramsCount = 1000;

        std::vector<DampedWaveFixtureParameters<cl_float>> params;
        std::mt19937 randomValueGenerator;
        auto rand = std::bind( std::uniform_real_distribution<cl_float>( 0.0f, 10.0f ), randomValueGenerator );
        std::generate_n( std::back_inserter( params ), paramsCount,
            [&rand]()
        {
            return DampedWaveFixtureParameters<cl_float>( rand(), 0.01f, rand(), rand() - 5.0f, rand() - 5.0f );
        } );

        {
            typedef std::normal_distribution<cl_float> Distribution;
            typedef RandomValuesIterator<cl_float, Distribution> Iterator;
            {
                auto fixture = std::make_shared<DampedWaveFixture<cl_float, Iterator>>( params,
                    Iterator( Distribution( 0.0f, 50.0f ) ), dataSize, "random input values" );
                fixtures.push_back( fixture );
                fixturesWithData.push_back( fixture );
            }
        }
        {
            typedef std::normal_distribution<cl_float> Distribution;
            typedef RandomValuesIterator<cl_float, Distribution> Iterator;
            auto fixture = std::make_shared<DampedWaveFixture<cl_float, SequentialValuesIterator<cl_float>>>( params,
                SequentialValuesIterator<cl_float>( min, step ), dataSize, "sequential input values" );
            fixtures.push_back( fixture );
            fixturesWithData.push_back( fixture );
        }
    }
}

std::vector<boost::compute::device> FixtureRunner::FillDevicesList()
{
    std::vector<boost::compute::device> result;
    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for( boost::compute::platform& platform : platforms )
    {
        auto devices = platform.devices();
        result.insert( result.end(), devices.cbegin(), devices.cend() );
    }
    return result;
}

std::unordered_map<cl_device_id, boost::compute::context> FixtureRunner::FillContextsMap(
    const std::vector<boost::compute::device>& devices )
{
    std::unordered_map<cl_device_id, boost::compute::context> result;
    for ( const boost::compute::device& device: devices )
    {
        result.insert( { device.get(), boost::compute::context( device ) } );
    }
    return result;
}

void FixtureRunner::Run( std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter, 
    FixtureRunner::FixturesToRun fixturesToRun )
{
    BOOST_LOG_TRIVIAL( info ) << "Welcome to OpenCL benchmark.";
    std::vector<std::shared_ptr<Fixture>> fixtures;
    std::vector<std::shared_ptr<FixtureThatReturnsData<cl_float>>> fixturesWithData;

    if (fixturesToRun.trivialFactorial)
    {
        CreateTrivialFixtures(fixtures);
    }
    if (fixturesToRun.dampedWave2D)
    {
        CreateDampedWave2DFixtures(fixtures, fixturesWithData);
    }

    BOOST_LOG_TRIVIAL( info ) << "We have " << fixtures.size() << " fixtures to run";

    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;
    auto targetFixtureExecutionTime = std::chrono::milliseconds( 10 ); // Time how long fixture should execute
    int minIterations = 10; // Minimal number of iterations

    std::vector<boost::compute::device> devicesList = FillDevicesList();
    std::unordered_map<cl_device_id, boost::compute::context> contextsList =
        FillContextsMap( devicesList );

    for( size_t fixtureIndex = 0; fixtureIndex < fixtures.size(); ++fixtureIndex )
    {
        std::shared_ptr<Fixture>& fixture = fixtures.at(fixtureIndex);
        fixture->Initialize();
        BenchmarkTimeWriterInterface::BenchmarkFixtureResultForFixture dataForTimeWriter;
        dataForTimeWriter.operationSteps = fixture->GetSteps();
        dataForTimeWriter.fixtureName = fixture->Description();
        dataForTimeWriter.elementsCount = fixture->GetElementsCount();

        for( boost::compute::device& device : devicesList )
        {
            dataForTimeWriter.perFixtureResults.push_back( BenchmarkTimeWriterInterface::BenchmarkFixtureResultForPlatform() );
            BenchmarkTimeWriterInterface::BenchmarkFixtureResultForPlatform& perPlatformResults = 
                dataForTimeWriter.perFixtureResults.back();

            perPlatformResults.platformName = device.platform().name();
            {
                perPlatformResults.perDeviceResults.push_back( BenchmarkTimeWriterInterface::BenchmarkFixtureResultForDevice() );
                BenchmarkTimeWriterInterface::BenchmarkFixtureResultForDevice& perDeviceResults =
                    perPlatformResults.perDeviceResults.back();
                perDeviceResults.deviceName = device.name();

                try
                {
                    {
                        std::vector<std::string> requiredExtensions = fixture->GetRequiredExtensions();
                        std::sort( requiredExtensions.begin(), requiredExtensions.end() );

                        std::vector<std::string> haveExtensions = device.extensions();
                        std::sort( haveExtensions.begin(), haveExtensions.end() );
                        if (!std::includes( haveExtensions.begin(), haveExtensions.end(), 
                            requiredExtensions.begin(), requiredExtensions.end() ) )
                        {
                            perDeviceResults.failureReason = "Required extension is not available";
                            continue;
                        }
                    }
                    boost::compute::context& context = contextsList.at( device.get() );
                    fixture->InitializeForContext( context );

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
                    // TODO replace with logging
                    BOOST_LOG_TRIVIAL( error ) << "OpenCL error occured: " << e.what();
                    if( e.error_code() == CL_BUILD_PROGRAM_FAILURE )
                    {
                        perDeviceResults.failureReason = "Kernel build failed";
                    }
                }
                catch( DataVerificationFailedException& e )
                {
                    BOOST_LOG_TRIVIAL( error ) << "Data verification failed: " << e.what();
                    perDeviceResults.failureReason = "Data verification failed";
                }
                catch( std::exception& e )
                {
                    BOOST_LOG_TRIVIAL( error ) << "Exception occured: " << e.what();
                }
            }
        }
        fixture->Finalize();
        timeWriter->WriteResultsForFixture( dataForTimeWriter );

        int fixturesLeft = fixtures.size() - fixtureIndex - 1;
        if( fixturesLeft > 0 )
        {
            BOOST_LOG_TRIVIAL( info ) << "Fixture \"" << fixture->Description() << "\" finished. "
                "Left " << fixturesLeft << " fixtures out of " << fixtures.size();
        }

        // Destroy fixture to release some memory sooner
        fixture.reset();
    }
    timeWriter->Flush();

    for (auto& fixtureWithData: fixturesWithData)
    {
        CSVDocument csvDocument( ( fixtureWithData->Description() + ".csv" ).c_str() );
        csvDocument.AddValues( fixtureWithData->GetResults() );
        csvDocument.BuildAndWriteToDisk();
    }
    BOOST_LOG_TRIVIAL( info ) << "Done";
}