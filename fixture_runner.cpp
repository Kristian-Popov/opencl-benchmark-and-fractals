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
#include "utils/duration.h"
#include "operation_step.h"
#include "half_precision_normal_distribution.h"
#include "program_build_failed_exception.h"

#include "documents/csv_document.h"
#include "documents/svg_document.h"

#include "devices/opencl_device.h"

#include "iterators/sequential_values_iterator.h"
#include "iterators/random_values_iterator.h"

#include "fixtures/damped_wave_opencl_fixture.h"
#include "fixtures/trivial_factorial_opencl_fixture.h"
#if 0
#include "fixtures/koch_curve_fixture.h"
#include "fixtures/multibrot_fractal_fixture.h"
#include "fixtures/multiprecision_factorial_fixture.h"
#endif

#include "fixtures/fixture_family.h"

#include <boost/algorithm/clamp.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/log/trivial.hpp>
#include <boost/math/constants/constants.hpp>
#include "half_precision_fp.h"

namespace
{
    template<typename T>
    struct TypeInfo
    {
        //static constexpr char* const description = "unknown";
    };

    template<>
    struct TypeInfo<float>
    {
        static constexpr char* const description = "float";
    };

    template<>
    struct TypeInfo<double>
    {
        static constexpr char* const description = "double";
    };

    template<>
    struct TypeInfo<half_float::half>
    {
        static constexpr const char* description = "half precision";
    };
}

void FixtureRunner::CreateTrivialFixtures()
{
    const std::vector<int32_t> data_sizes = { 100, 1000, 100000, 1000000 };
    for( int32_t s : data_sizes )
    {
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = "Trivial factorial, " + Utils::FormatQuantityString( s ) + " elements";
        fixture_family->operation_steps = {
            OperationStep::CopyInputDataToDevice,
            OperationStep::Calculate,
            OperationStep::CopyOutputDataFromDevice
        };
        fixture_family->element_count = s;
        for( auto& platform : platform_list_.OpenClPlatforms() )
        {
            for( auto& device : platform->GetDevices() )
            {
                typedef std::uniform_int_distribution<int> Distribution;
                typedef RandomValuesIterator<int, Distribution> Iterator;
                fixture_family->fixtures.insert( std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                    FixtureId( fixture_family->name, device, "" ),
                    std::make_shared<TrivialFactorialOpenClFixture>(
                        std::dynamic_pointer_cast<OpenClDevice>( device ),
                        std::make_shared<Iterator>( Distribution( 0, 20 ) ),
                        s
                    )
                ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
}

template<typename T>
void FixtureRunner::CreateDampedWave2DFixtures()
{
    T frequency = static_cast<T>( 1.0 );
    const T pi = boost::math::constants::pi<T>();

    T min = static_cast<T>( -10.0 );
    T max = static_cast<T>( 10.0 );
    T step = static_cast<T>( 0.001 );
    // TODO change to int32_t?
    size_t dataSize = static_cast<size_t>( ( max - min ) / step ); // TODO something similar can be useful in Utils
    {
        std::vector<DampedWaveFixtureParameters<T>> params = {DampedWaveFixtureParameters<T>{
            static_cast<T>( 1000.0 ),
            static_cast<T>( 0.1 ),
            static_cast<T>( 2 * pi*frequency ),
            static_cast<T>( 0.0 ),
            static_cast<T>( 1.0 )
        }};
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = ( boost::format( "Damped wave, %1%, %2% values, %3% parameters, sequential input data" ) %
            TypeInfo<T>::description %
            Utils::FormatQuantityString( dataSize ) %
            Utils::FormatQuantityString( params.size() ) ).str();
        fixture_family->operation_steps = {
            OperationStep::CopyInputDataToDevice,
            OperationStep::Calculate,
            OperationStep::CopyOutputDataFromDevice
        };
        fixture_family->element_count = dataSize;
        for( auto& platform : platform_list_.OpenClPlatforms() )
        {
            for( auto& device : platform->GetDevices() )
            {
                fixture_family->fixtures.insert( std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                    FixtureId( fixture_family->name, device, "" ),
                    std::make_shared<DampedWaveOpenClFixture<T>>(
                        std::dynamic_pointer_cast<OpenClDevice>( device ),
                        params,
                        std::make_shared<SequentialValuesIterator<T>>( min, step ),
                        dataSize,
                        fixture_family->name
                    )
                ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
#if 0
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
#endif
}
#if 0
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

void FixtureRunner::CreateMultiprecisionFactorialFixtures()
{
    std::vector<uint32_t> inputs = { 5, 10, 20, 50, 100, 200, 500, 1000, 10000 };
    std::transform( inputs.begin(), inputs.end(), std::back_inserter( fixtures_ ),
        []( uint32_t input )
    {
        return std::make_shared<MultiprecisionFactorialFixture>( input );
    } );
}
#endif
void FixtureRunner::SetFloatingPointEnvironment()
{
    int result = std::fesetround( FE_TONEAREST );
    EXCEPTION_ASSERT( result == 0);
}

void FixtureRunner::Clear()
{
    fixture_families_.clear();
}

void FixtureRunner::Run( std::unique_ptr<BenchmarkReporter> timeWriter, RunSettings settings )
{
    BOOST_LOG_TRIVIAL( info ) << "Welcome to OpenCL benchmark.";

    EXCEPTION_ASSERT( settings.minIterations >= 1 );
    EXCEPTION_ASSERT( settings.maxIterations >= 1 );

    SetFloatingPointEnvironment();

    timeWriter->Initialize( platform_list_ );

    Clear();
    FixturesToRun& fixturesToRun = settings.fixturesToRun;
    if (fixturesToRun.trivialFactorial)
    {
        CreateTrivialFixtures();
    }
    if( fixturesToRun.dampedWave2D )
    {
        CreateDampedWave2DFixtures<float>();
        CreateDampedWave2DFixtures<double>();
        CreateDampedWave2DFixtures<half_float::half>();
    }
#if 0
    if (fixturesToRun.kochCurve)
    {
        CreateKochCurveFixtures();
    }
    if (fixturesToRun.multibrotSet)
    {
        CreateMultibrotSetFixtures();
    }
    if (fixturesToRun.multiprecisionFactorial)
    {
        CreateMultiprecisionFactorialFixtures();
    }
#endif

    BOOST_LOG_TRIVIAL( info ) << "We have " << fixture_families_.size() << " fixture families to run";

    for( size_t familyIndex = 0; familyIndex < fixture_families_.size(); ++familyIndex )
    {
        std::shared_ptr<FixtureFamily>& fixtureFamily = fixture_families_.at( familyIndex );
        std::string fixtureName = fixtureFamily->name;
        BenchmarkResultForFixtureFamily results;
        results.fixture_family = fixtureFamily;

        BOOST_LOG_TRIVIAL( info ) << "Starting fixture family \"" << fixtureName << "\"";

        for( std::pair<const FixtureId, std::shared_ptr<Fixture>>& fixture_data: fixtureFamily->fixtures )
        {
            const FixtureId& fixture_id = fixture_data.first;
            std::shared_ptr<Fixture>& fixture = fixture_data.second;
            BenchmarkResultForFixture fixture_results;

            BOOST_LOG_TRIVIAL( info ) << "Starting run on device \"" << fixture_id.device()->Name() << "\"";

            try
            {
                {
                    std::vector<std::string> requiredExtensions = fixture->GetRequiredExtensions();
                    std::sort( requiredExtensions.begin(), requiredExtensions.end() );

                    std::vector<std::string> haveExtensions = fixture->Device()->Extensions();
                    std::sort( haveExtensions.begin(), haveExtensions.end() );

                    std::vector<std::string> missed_extensions;
                    std::set_difference(
                        requiredExtensions.cbegin(), requiredExtensions.cend(),
                        haveExtensions.cbegin(), haveExtensions.cend(),
                        std::back_inserter( missed_extensions )
                    );
                    if( !missed_extensions.empty() )
                    {
                        fixture_results.failure_reason = "Required extension(s) are not available";
                        // Destroy fixture to release some memory sooner
                        fixture.reset();

                        BOOST_LOG_TRIVIAL( warning ) << "Device \"" << fixture_id.device()->Name() <<
                            "\" doesn't support extensions needed for fixture: " << Utils::VectorToString( missed_extensions );

                        results.benchmark.insert( std::make_pair( fixture_id, fixture_results ) );

                        continue;
                    }
                }

                fixture->Initialize();

                std::vector<std::unordered_multimap<OperationStep, Duration>> durations;

                // Warm-up for one iteration to get estimation of execution time
                std::unordered_multimap<OperationStep, Duration> warmupResult = fixture->Execute();
                durations.push_back( warmupResult );
                Duration totalOperationDuration = std::accumulate( warmupResult.begin(), warmupResult.end(), Duration(),
                    []( Duration acc, const std::pair<OperationStep, Duration>& r )
                {
                    return acc + r.second;
                } );
                uint64_t iteration_count_long = settings.targetFixtureExecutionTime / totalOperationDuration;
                EXCEPTION_ASSERT( iteration_count_long < std::numeric_limits<int>::max() );
                int iteration_count = static_cast<int>( iteration_count_long );
                iteration_count = boost::algorithm::clamp( iteration_count, settings.minIterations, settings.maxIterations ) - 1;
                EXCEPTION_ASSERT( iteration_count >= 0 );

                if( settings.verifyResults )
                {
                    fixture->VerifyResults();
                }

                if( settings.storeResults )
                {
                    fixture->StoreResults();
                }

                for( int i = 0; i < iteration_count; ++i )
                {
                    durations.push_back( fixture->Execute() );
                }
                fixture_results.durations = durations;
            }
            catch( ProgramBuildFailedException& e )
            {
                std::stringstream stream;
                stream << "Program for fixture \"" <<
                    fixtureName << "\" failed to build on device \"" <<
                    e.DeviceName() << "\"";
                BOOST_LOG_TRIVIAL( error ) << stream.str();
                BOOST_LOG_TRIVIAL( info ) << "Build log: " << std::endl << e.BuildLog();
                BOOST_LOG_TRIVIAL( debug ) << e.what();
                fixture_results.failure_reason = "OpenCL Program failed to build";
            }
            catch( boost::compute::opencl_error& e )
            {
                BOOST_LOG_TRIVIAL( error ) << "OpenCL error occured: " << e.what();
                fixture_results.failure_reason = e.what();
            }
            catch( DataVerificationFailedException& e )
            {
                BOOST_LOG_TRIVIAL( error ) << "Data verification failed: " << e.what();
                fixture_results.failure_reason = "Data verification failed";
            }
            catch( std::exception& e )
            {
                BOOST_LOG_TRIVIAL( error ) << "Exception occured: " << e.what();
                fixture_results.failure_reason = e.what();
            }

            // Destroy fixture to release some memory sooner
            fixture.reset();

            BOOST_LOG_TRIVIAL( info ) << "Finished run on device \"" << fixture_id.device()->Name() << "\"";

            results.benchmark.insert( std::make_pair( fixture_id, fixture_results ) );
        }

        timeWriter->AddFixtureFamilyResults( results );

        BOOST_LOG_TRIVIAL( info ) << "Fixture family \"" << fixtureName << "\" finished successfully.";
    }
    timeWriter->Flush();

    BOOST_LOG_TRIVIAL( info ) << "Done";
}
