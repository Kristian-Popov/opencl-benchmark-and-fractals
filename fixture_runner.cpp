#include "fixture_runner.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <random>
#include <cfenv>

#include "data_verification_failed_exception.h"
#include "operation_step.h"
#include "csv_document.h"
#include "sequential_values_iterator.h"
#include "random_values_iterator.h"
#include "half_precision_normal_distribution.h"
#include "svg_document.h"
#include "program_build_failed_exception.h"

#include "trivial_factorial_fixture.h"
#include "damped_wave_fixture.h"
#include "koch_curve_fixture.h"
#include "fixtures/multibrot_fractal_fixture.h"

#include <boost/random/normal_distribution.hpp>
#include <boost/log/trivial.hpp>
#include <boost/compute.hpp>
#include <boost/math/constants/constants.hpp>
#include "half_precision_fp.h"

// TODO enable only on relevant compilers
#pragma STDC FENV_ACCESS on

void FixtureRunner::CreateTrivialFixtures()
{
    const std::vector<int> dataSizesForTrivialFactorial = {100, 1000, 100000, 1000000, 100000000};
    std::transform( dataSizesForTrivialFactorial.begin(), dataSizesForTrivialFactorial.end(), std::back_inserter( fixtures_ ),
        []( int dataSize )
    {
        typedef std::uniform_int_distribution<int> Distribution;
        typedef RandomValuesIterator<int, Distribution> Iterator;
        return std::make_shared<TrivialFactorialFixture<Iterator>>(
            Iterator( Distribution( 0, 20 ) ),
            dataSize );
    } );
}

void FixtureRunner::CreateDampedWave2DFixtures()
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
        fixtures_.push_back( dampedWaveFixture );
    }
    {
        std::vector<DampedWaveFixtureParameters<cl_double>> params =
        {DampedWaveFixtureParameters<cl_double>( 1000.0, 0.1, 2 * pi*frequency, 0.0, 1.0 )};

        typedef std::normal_distribution<cl_double> Distribution;
        typedef RandomValuesIterator<cl_double, Distribution> Iterator;
        auto dampedWaveFixture = std::make_shared<DampedWaveFixture<cl_double, Iterator>>( params,
            Distribution( 0.0, 100.0 ), dataSize, "random input values" );
        fixtures_.push_back( dampedWaveFixture );
    }
    {
        using namespace half_float::literal;
        std::vector<DampedWaveFixtureParameters<half_float::half>> params =
        {DampedWaveFixtureParameters<half_float::half>( 1000.0_h, 0.1_h, 
            static_cast<half_float::half>( 2.0_h * pi*frequency ), 0.0_h, 1.0_h )};

        typedef HalfPrecisionNormalDistribution Distribution;
        typedef RandomValuesIterator<half_float::half, Distribution> Iterator;
        auto dampedWaveFixture = std::make_shared<DampedWaveFixture<half_float::half, Iterator>>( params,
            Distribution( 0.0_h, 100.0_h ), dataSize, "random input values" );
        fixtures_.push_back( dampedWaveFixture );
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
                fixtures_.push_back( fixture );
            }
        }
        {
            typedef std::normal_distribution<cl_float> Distribution;
            typedef RandomValuesIterator<cl_float, Distribution> Iterator;
            auto fixture = std::make_shared<DampedWaveFixture<cl_float, SequentialValuesIterator<cl_float>>>( params,
                SequentialValuesIterator<cl_float>( min, step ), dataSize, "sequential input values" );
            fixtures_.push_back( fixture );
        }
    }
}

void FixtureRunner::CreateKochCurveFixtures()
{
    // TODO it would be great to get images with higher number of iterations but
    // another output method is needed (SVG doesn't work well)
    const std::vector<int> iterationCountVariants = {1, 3, 7};
    struct CurveVariant
    {
        std::vector<cl_double4> curves;
        std::string descriptionSuffix;
    };
    std::vector<CurveVariant> curveVariants;
    {
        std::vector<cl_double4> singleCurve = {{0.0, 0.0, 1000.0, 0.0}};
        std::vector<cl_double4> twoCurvesFace2Face = {
            {0.0, 0.0, 1000.0, 0.0},
            {1000.0, 300.0, 0.0, 300.0}
        };
        std::vector<cl_double4> snowflakeTriangleCurves;
        {
            cl_double2 A = { 300.0, 646.41 };
            cl_double2 B = { 500.0, 300.0 };
            cl_double2 C = { 700.0, 646.41 };
            snowflakeTriangleCurves = {
                Utils::CombineTwoDouble2Vectors(B, A),
                Utils::CombineTwoDouble2Vectors(C, B),
                Utils::CombineTwoDouble2Vectors(A, C)
            };
        }
        std::vector<cl_double4> snowflakeSomeFigure = {
            {300.0, 646.41, 500.0, 300.0},
            {700.0, 646.41, 300.0, 646.41},
            {500.0, 300.0, 700.0, 646.41},
        };
        std::vector<cl_double4> snowflakeSquareCurves;
        {
            cl_double2 A = {300.0, 300.0};
            cl_double2 B = {700.0, 300.0};
            cl_double2 C = {300.0, 700.0};
            cl_double2 D = {700.0, 700.0};
            snowflakeSquareCurves = {
                Utils::CombineTwoDouble2Vectors( B, A ),
                Utils::CombineTwoDouble2Vectors( A, C ),
                Utils::CombineTwoDouble2Vectors( D, B ),
                Utils::CombineTwoDouble2Vectors( C, D ),
            };
        }
        curveVariants = { 
            { singleCurve, "single curve" }, 
            { twoCurvesFace2Face, "two curves" },
            { snowflakeTriangleCurves, "triangle" },
            { snowflakeSquareCurves, "square"},
            { snowflakeSomeFigure, "some figure"}
        };
    }
    
    for (int i: iterationCountVariants)
    {
        for (const auto& curveVariant: curveVariants)
        {
            {
                auto ptr = std::make_shared<KochCurveFixture<cl_float,
                    cl_float2, cl_float4>>( i, Utils::ConvertDouble4ToFloat4Vectors( curveVariant.curves ),
                        1000.0f, 1000.0f, curveVariant.descriptionSuffix );
                fixtures_.push_back( ptr );
            }
            {
                auto ptr = std::make_shared<KochCurveFixture<cl_double,
                    cl_double2, cl_double4>>( i, curveVariant.curves, 1000.0, 1000.0, 
                        curveVariant.descriptionSuffix );
                fixtures_.push_back( ptr );
            }
        }
    }
}

void FixtureRunner::CreateMultibrotSetFixtures()
{
    // TODO add support for negative powers
    std::vector<double> powers = { 1.0, 2.0, 3.0, 7.0, 3.5, 0.1, 0.5 };
    std::array<double, 2> realRange = {-2.5, 1.5}, imgRange = {-2.0, 2.0};

    for (double power: powers)
    {
        {
            auto ptr = std::make_shared<MultibrotFractalFixture<cl_float>>(
                3000, 3000, realRange.at(0), realRange.at(1), imgRange.at(0), imgRange.at(1), power );
            fixtures_.push_back( ptr );
        }
        {
            auto ptr = std::make_shared<MultibrotFractalFixture<cl_double>>(
                2000, 2000, realRange.at(0), realRange.at(1), imgRange.at(0), imgRange.at(1), power );
            fixtures_.push_back( ptr );
        }
        {
            auto ptr = std::make_shared<MultibrotFractalFixture<half_float::half>>(
                1000, 1000, 
                static_cast<half_float::half>( realRange.at( 0 )), 
                static_cast<half_float::half>( realRange.at( 1 )),
                static_cast<half_float::half>( imgRange.at( 0 )),
                static_cast<half_float::half>( imgRange.at( 1 )),
                static_cast<half_float::half>( power ));
            fixtures_.push_back( ptr );
        }
    }
}

void FixtureRunner::SetFloatingPointEnvironment()
{
    int result = std::fesetround( FE_TONEAREST );
    EXCEPTION_ASSERT( result == 0);
}

void FixtureRunner::FillContextsMap()
{
    for ( const boost::compute::platform& platform: boost::compute::system::platforms() )
    {
        for ( const boost::compute::device& device: platform.devices() )
        {
            contexts_.insert( { device.get(), boost::compute::context( device ) } );
        }
    }
}
void FixtureRunner::Clear()
{
    fixtures_.clear();
    contexts_.clear();
}

void FixtureRunner::Run( std::unique_ptr<BenchmarkTimeWriterInterface> timeWriter, 
    FixtureRunner::FixturesToRun fixturesToRun )
{
    BOOST_LOG_TRIVIAL( info ) << "Welcome to OpenCL benchmark.";

    SetFloatingPointEnvironment();

    Clear();
    if (fixturesToRun.trivialFactorial)
    {
        CreateTrivialFixtures();
    }
    if (fixturesToRun.dampedWave2D)
    {
        CreateDampedWave2DFixtures();
    }
    if (fixturesToRun.kochCurve)
    {
        CreateKochCurveFixtures();
    }
    if (fixturesToRun.multibrotSet)
    {
        CreateMultibrotSetFixtures();
    }

    BOOST_LOG_TRIVIAL( info ) << "We have " << fixtures_.size() << " fixtures to run";

    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;
    auto targetFixtureExecutionTime = std::chrono::milliseconds( 10 ); // Time how long fixture should execute
    int minIterations = 10; // Minimal number of iterations

    FillContextsMap();

    for( size_t fixtureIndex = 0; fixtureIndex < fixtures_.size(); ++fixtureIndex )
    {
        std::shared_ptr<Fixture>& fixture = fixtures_.at(fixtureIndex);
        
        std::string fixtureName = fixture->Description();

        fixture->Initialize();
        BenchmarkTimeWriterInterface::BenchmarkFixtureResultForFixture dataForTimeWriter;
        dataForTimeWriter.operationSteps = fixture->GetSteps();
        dataForTimeWriter.fixtureName = fixtureName;
        dataForTimeWriter.elementsCount = fixture->GetElementsCount();

        for( boost::compute::platform& platform : boost::compute::system::platforms() )
        {
            dataForTimeWriter.perFixtureResults.push_back( BenchmarkTimeWriterInterface::BenchmarkFixtureResultForPlatform() );
            BenchmarkTimeWriterInterface::BenchmarkFixtureResultForPlatform& perPlatformResults = 
                dataForTimeWriter.perFixtureResults.back();

            perPlatformResults.platformName = platform.name();
            for( const boost::compute::device& device : platform.devices() )
            {
                // Find corresponding context
                boost::compute::context& context = contexts_.at( device.get() );

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
                            perDeviceResults.failureReason = "Required extension(s) are not available";
                            continue;
                        }
                    }
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
                    //iterationCount = std::max( iterationCount, minIterations );
                    iterationCount = 1;
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
                catch( ProgramBuildFailedException& e )
                {
                    BOOST_LOG_TRIVIAL( error ) << "Program for fixture \"" <<
                        fixtureName << "\" failed to build on device \"" <<
                        e.DeviceName() << "\"";
                    BOOST_LOG_TRIVIAL( debug ) << e.what();
                }
                catch( boost::compute::opencl_error& e )
                {
                    // TODO replace with logging
                    BOOST_LOG_TRIVIAL( error ) << "OpenCL error occured: " << e.what();
                    perDeviceResults.failureReason = e.what();
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

        int fixturesLeft = fixtures_.size() - fixtureIndex - 1;
        if( fixturesLeft > 0 )
        {
            BOOST_LOG_TRIVIAL( info ) << "Fixture \"" << fixture->Description() << "\" finished. "
                "Left " << fixturesLeft << " fixtures out of " << fixtures_.size();
        }

        try
        {
            fixture->WriteResults();
        }
        catch( std::exception& e )
        {
            //BOOST_LOG_TRIVIAL( error ) << "Caught exception when building SVG document: " << e.what();
            BOOST_LOG_TRIVIAL( error ) << "Error when writing fixture \"" << fixtureName << "\" results: " << e.what();
        }

        // Destroy fixture to release some memory sooner
        fixture.reset();
    }
    timeWriter->Flush();

    BOOST_LOG_TRIVIAL( info ) << "Done";
}