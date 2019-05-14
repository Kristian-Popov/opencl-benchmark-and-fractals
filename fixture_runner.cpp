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
#include "fixtures/koch_curve_opencl_fixture.h"
#include "fixtures/multibrot_opencl_fixture.h"

#include "fixtures/fixture_family.h"

#include <boost/algorithm/clamp.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/log/trivial.hpp>
#include <boost/math/constants/constants.hpp>
#include "half_precision_fp.h"

namespace
{
    // TODO it is a template specialization but other places use other structures use other methods,
    // consolidate them somehow?
    template<typename T>
    struct MultibrotSetParams
    {
        // static constexpr const size_t width_pix;
        // static constexpr const size_t height_pix;
    };

    template<>
    struct MultibrotSetParams<half_float::half>
    {
        static constexpr const size_t width_pix = 1000;
        static constexpr const size_t height_pix = 1000;
    };

    template<>
    struct MultibrotSetParams<float>
    {
        static constexpr const size_t width_pix = 2000;
        static constexpr const size_t height_pix = 2000;
    };

    template<>
    struct MultibrotSetParams<double>
    {
        static constexpr const size_t width_pix = 3000;
        static constexpr const size_t height_pix = 3000;
    };

    template<typename P>
    struct MultibrotResultConstants
    {
        // static constexpr const char* pixel_type_description;
    };

    template<>
    struct MultibrotResultConstants<cl_uchar>
    {
        static constexpr const char* pixel_type_description = "grayscale 8 bit";
    };

    template<>
    struct MultibrotResultConstants<cl_ushort>
    {
        static constexpr const char* pixel_type_description = "grayscale 16 bit";
    };
}

void FixtureRunner::CreateTrivialFixtures()
{
    const std::vector<int32_t> data_sizes = { 100, 1000, 100000, 1000000 };
    for( int32_t s : data_sizes )
    {
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = "Trivial factorial, " + Utils::FormatQuantityString( s ) + " elements";
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

template<typename T, typename D = std::normal_distribution<T>>
void FixtureRunner::CreateDampedWave2DFixtures()
{
    T frequency = static_cast<T>( 1.0 );
    const T pi = boost::math::constants::pi<T>();

    T min = static_cast<T>( -10.0 );
    T max = static_cast<T>( 10.0 );
    T step = static_cast<T>( 0.001 );
    // TODO change to int32_t?
    size_t data_size = static_cast<size_t>( ( max - min ) / step ); // TODO something similar can be useful in Utils

    std::mt19937 randomValueGenerator;
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
            OpenClTypeTraits<T>::short_description %
            Utils::FormatQuantityString( data_size ) %
            Utils::FormatQuantityString( params.size() ) ).str();
        fixture_family->element_count = data_size;
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
                        data_size,
                        fixture_family->name
                    )
                ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
    {
        std::vector<DampedWaveFixtureParameters<T>> params = {DampedWaveFixtureParameters<T>{
            static_cast<T>( 1000.0 ),
            static_cast<T>( 0.1 ),
            static_cast<T>( 2 * pi*frequency ),
            static_cast<T>( 0.0 ),
            static_cast<T>( 1.0 )
        }};
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = ( boost::format( "Damped wave, %1%, %2% values, %3% parameters, random input data" ) %
            OpenClTypeTraits<T>::short_description %
            Utils::FormatQuantityString( data_size ) %
            Utils::FormatQuantityString( params.size() ) ).str();
        fixture_family->element_count = data_size;
        for( auto& platform : platform_list_.OpenClPlatforms() )
        {
            for( auto& device : platform->GetDevices() )
            {
                fixture_family->fixtures.insert( std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                    FixtureId( fixture_family->name, device, "" ),
                    std::make_shared<DampedWaveOpenClFixture<T>>(
                        std::dynamic_pointer_cast<OpenClDevice>( device ),
                        params,
                        std::make_shared<RandomValuesIterator<T, D>>( D(
                            static_cast<T>( 0.0 ),
                            static_cast<T>( 100.0 )
                        ) ),
                        data_size,
                        fixture_family->name
                        )
                    ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
    {
        const size_t paramsCount = 1000;
        std::vector<DampedWaveFixtureParameters<T>> params;
        auto rand = std::bind( D( static_cast<T>( 0.0 ), static_cast<T>( 10.0 ) ), randomValueGenerator );
        std::generate_n( std::back_inserter( params ), paramsCount,
            [&rand]()
        {
            return DampedWaveFixtureParameters<T>(
                rand(),
                static_cast<T>( 0.01 ),
                rand(),
                rand() - static_cast<T>( 5.0 ),
                rand() - static_cast<T>( 5.0 )
            );
        } );
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = ( boost::format( "Damped wave, %1%, %2% values, %3% parameters, sequential input data" ) %
            OpenClTypeTraits<T>::short_description %
            Utils::FormatQuantityString( data_size ) %
            Utils::FormatQuantityString( params.size() ) ).str();
        fixture_family->element_count = data_size;
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
                        data_size,
                        fixture_family->name
                        )
                    ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
    {
        const size_t paramsCount = 1000;
        std::vector<DampedWaveFixtureParameters<T>> params;
        auto rand = std::bind( D( static_cast<T>( 0.0 ), static_cast<T>( 10.0 ) ), randomValueGenerator );
        std::generate_n( std::back_inserter( params ), paramsCount,
            [&rand]()
        {
            return DampedWaveFixtureParameters<T>(
                rand(),
                static_cast<T>( 0.01 ),
                rand(),
                rand() - static_cast<T>( 5.0 ),
                rand() - static_cast<T>( 5.0 )
                );
            } );
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = ( boost::format( "Damped wave, %1%, %2% values, %3% parameters, random input data" ) %
            OpenClTypeTraits<T>::short_description %
            Utils::FormatQuantityString( data_size ) %
            Utils::FormatQuantityString( params.size() ) ).str();
        fixture_family->element_count = data_size;
        for( auto& platform : platform_list_.OpenClPlatforms() )
        {
            for( auto& device : platform->GetDevices() )
            {
                fixture_family->fixtures.insert( std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                    FixtureId( fixture_family->name, device, "" ),
                    std::make_shared<DampedWaveOpenClFixture<T>>(
                        std::dynamic_pointer_cast<OpenClDevice>( device ),
                        params,
                        std::make_shared<RandomValuesIterator<T, D>>( D(
                            static_cast<T>( 0.0 ),
                            static_cast<T>( 100.0 )
                        ) ),
                        data_size,
                        fixture_family->name
                        )
                    ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
}

template<typename T, typename T4>
void FixtureRunner::CreateKochCurveFixtures()
{
    // TODO it would be great to get images with higher number of iterations but
    // another output method is needed (SVG doesn't work well)
    const std::vector<int> iteration_count_variants = {1, 3, 7};
    struct CurveVariant
    {
        std::vector<cl_double4> curves;
        std::string description;
    };

    std::vector<CurveVariant> curve_variants;
    {
        std::vector<cl_double4> singleCurve = {{0.0, 0.0, 1000.0, 0.0}};
        std::vector<cl_double4> twoCurvesFace2Face = {
            {0.0, 0.0, 1000.0, 0.0},
            {1000.0, 300.0, 0.0, 300.0}
        };
        std::vector<cl_double4> snowflakeTriangleCurves;
        {
            cl_double2 A = {300.0, 646.41};
            cl_double2 B = {500.0, 300.0};
            cl_double2 C = {700.0, 646.41};
            snowflakeTriangleCurves = {
                Utils::CombineTwoDouble2Vectors( B, A ),
                Utils::CombineTwoDouble2Vectors( C, B ),
                Utils::CombineTwoDouble2Vectors( A, C )
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
        curve_variants = {
            {singleCurve, "single curve"},
            {twoCurvesFace2Face, "two curves"},
            {snowflakeTriangleCurves, "triangle"},
            {snowflakeSquareCurves, "square"},
            {snowflakeSomeFigure, "some figure"}
        };
    }

    for( int iterations : iteration_count_variants )
    {
        for( const auto& curve_variant : curve_variants )
        {
            auto fixture_family = std::make_shared<FixtureFamily>();
            fixture_family->name = ( boost::format( "Koch curve, %1%, %2% iterations, %3%" ) %
                OpenClTypeTraits<T>::short_description %
                iterations %
                curve_variant.description ).str();
            // TODO fixture_family->element_count can be calculated but is not trivial

            std::vector<T4> casted_curves;
            std::transform( curve_variant.curves.cbegin(), curve_variant.curves.cend(),
                std::back_inserter( casted_curves ),
                []( const cl_double4& v ) -> T4
            {
                return Utils::StaticCastVector4<T4, cl_double4>( v );
            } );

            for( auto& platform : platform_list_.OpenClPlatforms() )
            {
                for( auto& device : platform->GetDevices() )
                {
                    fixture_family->fixtures.insert( std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                        FixtureId( fixture_family->name, device, "" ),
                        std::make_shared<KochCurveOpenClFixture<T, T4>>(
                            std::dynamic_pointer_cast<OpenClDevice>( device ),
                            iterations, casted_curves,
                            1000.0, 1000.0,
                            fixture_family->name
                        )
                    ) );
                }
            }
            fixture_families_.push_back( fixture_family );
        }
    }
}

template<typename T, typename P>
void FixtureRunner::CreateMultibrotSetFixtures()
{
    std::vector<double> powers = { 1.0, 2.0, 3.0, 7.0, 3.5, 0.1, 0.5 };
    std::complex<double> min{ -2.5, -2.0 };
    std::complex<double> max{ 1.5, 2.0 };
    for( double power: powers )
    {
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = ( boost::format( "%1%, %3%, %4%" ) %
            ( ( power == 2.0 ) ? std::string( "Mandelbrot set" ) : "Multibrot set, power " + std::to_string( power ) ) %
            power %
            OpenClTypeTraits<T>::short_description %
            MultibrotResultConstants<P>::pixel_type_description ).str();
        fixture_family->element_count = MultibrotSetParams<T>::width_pix * MultibrotSetParams<T>::height_pix;

        for( auto& platform : platform_list_.OpenClPlatforms() )
        {
            for( auto& device : platform->GetDevices() )
            {
                fixture_family->fixtures.insert( std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                    FixtureId( fixture_family->name, device, "" ),
                    std::make_shared<MultibrotOpenClFixture<T, P>>(
                            std::dynamic_pointer_cast<OpenClDevice>( device ),
                            MultibrotSetParams<T>::width_pix, MultibrotSetParams<T>::height_pix,
                            min, max,
                            power,
                            fixture_family->name
                        )
                ) );
            }
        }
        fixture_families_.push_back( fixture_family );
    }
}

void FixtureRunner::Clear()
{
    fixture_families_.clear();
}

void FixtureRunner::Run( std::unique_ptr<BenchmarkReporter> reporter, RunSettings settings )
{
    BOOST_LOG_TRIVIAL( info ) << "Welcome to OpenCL benchmark.";

    EXCEPTION_ASSERT( settings.min_iterations >= 1 );
    EXCEPTION_ASSERT( settings.max_iterations >= 1 );

    reporter->Initialize( platform_list_ );

    Clear();
    FixturesToRun& fixtures_to_run = settings.fixtures_to_run;
    if (fixtures_to_run.trivial_factorial)
    {
        CreateTrivialFixtures();
    }
    if (fixtures_to_run.damped_wave)
    {
        CreateDampedWave2DFixtures<float>();
        CreateDampedWave2DFixtures<double>();
        CreateDampedWave2DFixtures<half_float::half, HalfPrecisionNormalDistribution>();
    }
    if (fixtures_to_run.koch_curve)
    {
        CreateKochCurveFixtures<float, cl_float4>();
        CreateKochCurveFixtures<double, cl_double4>();
        //CreateKochCurveFixtures<half_float::half, half_float::half[4]>(); // TODO enable
    }
    if (fixtures_to_run.multibrot_set)
    {
        CreateMultibrotSetFixtures<half_float::half, cl_uchar>();
        CreateMultibrotSetFixtures<float, cl_uchar>();
        CreateMultibrotSetFixtures<double, cl_uchar>();

        CreateMultibrotSetFixtures<half_float::half, cl_ushort>();
        CreateMultibrotSetFixtures<float, cl_ushort>();
        CreateMultibrotSetFixtures<double, cl_ushort>();
    }

    BOOST_LOG_TRIVIAL( info ) << "We have " << fixture_families_.size() << " fixture families to run";

    for( size_t family_index = 0; family_index < fixture_families_.size(); ++family_index )
    {
        std::shared_ptr<FixtureFamily>& fixture_family = fixture_families_.at( family_index );
        std::string fixture_name = fixture_family->name;
        FixtureFamilyBenchmark results;
        results.fixture_family = fixture_family;

        BOOST_LOG_TRIVIAL( info ) << "Starting fixture family \"" << fixture_name << "\"";

        for( std::pair<const FixtureId, std::shared_ptr<Fixture>>& fixture_data: fixture_family->fixtures )
        {
            const FixtureId& fixture_id = fixture_data.first;
            std::shared_ptr<Fixture>& fixture = fixture_data.second;
            FixtureBenchmark fixture_results;

            BOOST_LOG_TRIVIAL( info ) << "Starting run on device \"" << fixture_id.device()->Name() << "\"";

            try
            {
                {
                    std::vector<std::string> required_extensions = fixture->GetRequiredExtensions();
                    std::sort( required_extensions.begin(), required_extensions.end() );

                    std::vector<std::string> have_extensions = fixture->Device()->Extensions();
                    std::sort( have_extensions.begin(), have_extensions.end() );

                    std::vector<std::string> missed_extensions;
                    std::set_difference(
                        required_extensions.cbegin(), required_extensions.cend(),
                        have_extensions.cbegin(), have_extensions.cend(),
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

                std::vector<std::unordered_map<OperationStep, Duration>> durations;

                // Warm-up for one iteration to get estimation of execution time
                std::unordered_map<OperationStep, Duration> warmup_result = fixture->Execute();
                durations.push_back( warmup_result );
                Duration total_operation_duration = std::accumulate( warmup_result.begin(), warmup_result.end(), Duration(),
                    []( Duration acc, const std::pair<OperationStep, Duration>& r )
                {
                    return acc + r.second;
                } );
                uint64_t iteration_count_long = settings.target_fixture_execution_time / total_operation_duration;
                EXCEPTION_ASSERT( iteration_count_long < std::numeric_limits<int>::max() );
                int iteration_count = static_cast<int>( iteration_count_long );
                iteration_count = boost::algorithm::clamp( iteration_count, settings.min_iterations, settings.max_iterations ) - 1;
                EXCEPTION_ASSERT( iteration_count >= 0 );

                if (settings.verify_results)
                {
                    fixture->VerifyResults();
                }

                if (settings.store_results)
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
                    fixture_name << "\" failed to build on device \"" <<
                    e.DeviceName() << "\"";
                BOOST_LOG_TRIVIAL( error ) << stream.str();
                BOOST_LOG_TRIVIAL( info ) << "Build options: " << e.BuildOptions();
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

        reporter->AddFixtureFamilyResults( results );

        BOOST_LOG_TRIVIAL( info ) << "Fixture family \"" << fixture_name << "\" finished successfully.";
    }
    reporter->Flush();

    BOOST_LOG_TRIVIAL( info ) << "Done";
}
