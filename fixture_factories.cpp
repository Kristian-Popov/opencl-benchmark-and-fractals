#include <boost/math/constants/constants.hpp>
#include <boost/random/normal_distribution.hpp>

#include "devices/platform_list.h"
#include "documents/csv_document.h"
#include "documents/svg_document.h"
#include "fixture_register_macros.h"
#include "fixtures/damped_wave_opencl_fixture.h"
#include "fixtures/fixture_family.h"
#include "fixtures/koch_curve_opencl_fixture.h"
#include "fixtures/multibrot_opencl_fixture.h"
#include "fixtures/trivial_factorial_opencl_fixture.h"
#include "half_precision_fp.h"
#include "half_precision_normal_distribution.h"
#include "iterators/random_values_iterator.h"
#include "iterators/sequential_values_iterator.h"

#if 0
std::vector<std::shared_ptr<FixtureFamily>> CreateTrivialFixtures(
    const PlatformList& platform_list);
template <typename T, typename D = std::normal_distribution<T>>
std::vector<std::shared_ptr<FixtureFamily>> CreateDampedWave2DFixtures(
    const PlatformList& platform_list);
template <typename T, typename T4>
std::vector<std::shared_ptr<FixtureFamily>> CreateKochCurveFixtures(
    const PlatformList& platform_list);
template <typename T, typename P>
std::vector<std::shared_ptr<FixtureFamily>> FixtureRunner::CreateMultibrotSetFixtures(
    const PlatformList& platform_list);
#endif

namespace {
// TODO it is a template specialization but other places use other methods, consolidate them
// somehow?
template <typename T>
struct MultibrotSetParams {
    // static constexpr const size_t width_pix;
    // static constexpr const size_t height_pix;
};

template <>
struct MultibrotSetParams<half_float::half> {
    static constexpr const size_t width_pix = 1000;
    static constexpr const size_t height_pix = 1000;
};

template <>
struct MultibrotSetParams<float> {
    static constexpr const size_t width_pix = 2000;
    static constexpr const size_t height_pix = 2000;
};

template <>
struct MultibrotSetParams<double> {
    static constexpr const size_t width_pix = 3000;
    static constexpr const size_t height_pix = 3000;
};

template <typename P>
struct MultibrotResultConstants {
    // static constexpr const char* pixel_type_description;
};

template <>
struct MultibrotResultConstants<cl_uchar> {
    static constexpr const char* pixel_type_description = "grayscale 8 bit";
};

template <>
struct MultibrotResultConstants<cl_ushort> {
    static constexpr const char* pixel_type_description = "grayscale 16 bit";
};
}  // namespace

std::shared_ptr<FixtureFamily> CreateTrivialFactorialFixtures(
    const kpv::PlatformList& platform_list, int32_t data_size) {
    auto fixture_family = std::make_shared<FixtureFamily>();
    fixture_family->name =
        "Trivial factorial, " + Utils::FormatQuantityString(data_size) + " elements";
    fixture_family->element_count = data_size;
    for (auto& platform : platform_list.OpenClPlatforms()) {
        for (auto& device : platform->GetDevices()) {
            typedef std::uniform_int_distribution<int> Distribution;
            typedef RandomValuesIterator<int, Distribution> Iterator;
            fixture_family->fixtures.insert(
                std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                    FixtureId(fixture_family->name, device, ""),
                    std::make_shared<TrivialFactorialOpenClFixture>(
                        std::dynamic_pointer_cast<OpenClDevice>(device),
                        std::make_shared<Iterator>(Distribution(0, 20)), data_size)));
        }
    }
    return fixture_family;
}

REGISTER_FIXTURE(
    "trivial-factorial", std::bind(&CreateTrivialFactorialFixtures, ::std::placeholders::_1, 100));
REGISTER_FIXTURE(
    "trivial-factorial", std::bind(&CreateTrivialFactorialFixtures, ::std::placeholders::_1, 1000));
REGISTER_FIXTURE(
    "trivial-factorial",
    std::bind(&CreateTrivialFactorialFixtures, ::std::placeholders::_1, 100000));
REGISTER_FIXTURE(
    "trivial-factorial",
    std::bind(&CreateTrivialFactorialFixtures, ::std::placeholders::_1, 1000000));

#if 0
template <typename T, typename D = std::normal_distribution<T>>
std::vector<std::shared_ptr<FixtureFamily>> CreateDampedWave2DFixtures(
    const PlatformList& platform_list) {
    T frequency = static_cast<T>(1.0);
    const T pi = boost::math::constants::pi<T>();

    T min = static_cast<T>(-10.0);
    T max = static_cast<T>(10.0);
    T step = static_cast<T>(0.001);
    // TODO change to int32_t?
    size_t data_size =
        static_cast<size_t>((max - min) / step);  // TODO something similar can be useful in Utils

    std::mt19937 randomValueGenerator;
    {
        std::vector<DampedWaveFixtureParameters<T>> params = {DampedWaveFixtureParameters<T>{
            static_cast<T>(1000.0), static_cast<T>(0.1), static_cast<T>(2 * pi * frequency),
            static_cast<T>(0.0), static_cast<T>(1.0)}};
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name =
            (boost::format("Damped wave, %1%, %2% values, %3% parameters, sequential input data") %
             OpenClTypeTraits<T>::short_description % Utils::FormatQuantityString(data_size) %
             Utils::FormatQuantityString(params.size()))
                .str();
        fixture_family->element_count = data_size;
        for (auto& platform : platform_list_.OpenClPlatforms()) {
            for (auto& device : platform->GetDevices()) {
                fixture_family->fixtures.insert(
                    std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                        FixtureId(fixture_family->name, device, ""),
                        std::make_shared<DampedWaveOpenClFixture<T>>(
                            std::dynamic_pointer_cast<OpenClDevice>(device), params,
                            std::make_shared<SequentialValuesIterator<T>>(min, step), data_size,
                            fixture_family->name)));
            }
        }
        fixture_families_.push_back(fixture_family);
    }
    {
        std::vector<DampedWaveFixtureParameters<T>> params = {DampedWaveFixtureParameters<T>{
            static_cast<T>(1000.0), static_cast<T>(0.1), static_cast<T>(2 * pi * frequency),
            static_cast<T>(0.0), static_cast<T>(1.0)}};
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name =
            (boost::format("Damped wave, %1%, %2% values, %3% parameters, random input data") %
             OpenClTypeTraits<T>::short_description % Utils::FormatQuantityString(data_size) %
             Utils::FormatQuantityString(params.size()))
                .str();
        fixture_family->element_count = data_size;
        for (auto& platform : platform_list_.OpenClPlatforms()) {
            for (auto& device : platform->GetDevices()) {
                fixture_family->fixtures.insert(
                    std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                        FixtureId(fixture_family->name, device, ""),
                        std::make_shared<DampedWaveOpenClFixture<T>>(
                            std::dynamic_pointer_cast<OpenClDevice>(device), params,
                            std::make_shared<RandomValuesIterator<T, D>>(
                                D(static_cast<T>(0.0), static_cast<T>(100.0))),
                            data_size, fixture_family->name)));
            }
        }
        fixture_families_.push_back(fixture_family);
    }
    {
        const size_t paramsCount = 1000;
        std::vector<DampedWaveFixtureParameters<T>> params;
        auto rand = std::bind(D(static_cast<T>(0.0), static_cast<T>(10.0)), randomValueGenerator);
        std::generate_n(std::back_inserter(params), paramsCount, [&rand]() {
            return DampedWaveFixtureParameters<T>(
                rand(), static_cast<T>(0.01), rand(), rand() - static_cast<T>(5.0),
                rand() - static_cast<T>(5.0));
        });
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name =
            (boost::format("Damped wave, %1%, %2% values, %3% parameters, sequential input data") %
             OpenClTypeTraits<T>::short_description % Utils::FormatQuantityString(data_size) %
             Utils::FormatQuantityString(params.size()))
                .str();
        fixture_family->element_count = data_size;
        for (auto& platform : platform_list_.OpenClPlatforms()) {
            for (auto& device : platform->GetDevices()) {
                fixture_family->fixtures.insert(
                    std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                        FixtureId(fixture_family->name, device, ""),
                        std::make_shared<DampedWaveOpenClFixture<T>>(
                            std::dynamic_pointer_cast<OpenClDevice>(device), params,
                            std::make_shared<SequentialValuesIterator<T>>(min, step), data_size,
                            fixture_family->name)));
            }
        }
        fixture_families_.push_back(fixture_family);
    }
    {
        const size_t paramsCount = 1000;
        std::vector<DampedWaveFixtureParameters<T>> params;
        auto rand = std::bind(D(static_cast<T>(0.0), static_cast<T>(10.0)), randomValueGenerator);
        std::generate_n(std::back_inserter(params), paramsCount, [&rand]() {
            return DampedWaveFixtureParameters<T>(
                rand(), static_cast<T>(0.01), rand(), rand() - static_cast<T>(5.0),
                rand() - static_cast<T>(5.0));
        });
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name =
            (boost::format("Damped wave, %1%, %2% values, %3% parameters, random input data") %
             OpenClTypeTraits<T>::short_description % Utils::FormatQuantityString(data_size) %
             Utils::FormatQuantityString(params.size()))
                .str();
        fixture_family->element_count = data_size;
        for (auto& platform : platform_list_.OpenClPlatforms()) {
            for (auto& device : platform->GetDevices()) {
                fixture_family->fixtures.insert(
                    std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                        FixtureId(fixture_family->name, device, ""),
                        std::make_shared<DampedWaveOpenClFixture<T>>(
                            std::dynamic_pointer_cast<OpenClDevice>(device), params,
                            std::make_shared<RandomValuesIterator<T, D>>(
                                D(static_cast<T>(0.0), static_cast<T>(100.0))),
                            data_size, fixture_family->name)));
            }
        }
        fixture_families_.push_back(fixture_family);
    }
}

template <typename T, typename T4>
std::vector<std::shared_ptr<FixtureFamily>> CreateKochCurveFixtures(
    const PlatformList& platform_list) {
    // TODO it would be great to get images with higher number of iterations but
    // another output method is needed (SVG doesn't work well)
    const std::vector<int> iteration_count_variants = {1, 3, 7};
    struct CurveVariant {
        std::vector<cl_double4> curves;
        std::string description;
    };

    std::vector<CurveVariant> curve_variants;
    {
        std::vector<cl_double4> singleCurve = {{0.0, 0.0, 1000.0, 0.0}};
        std::vector<cl_double4> twoCurvesFace2Face = {{0.0, 0.0, 1000.0, 0.0},
                                                      {1000.0, 300.0, 0.0, 300.0}};
        std::vector<cl_double4> snowflakeTriangleCurves;
        {
            cl_double2 A = {300.0, 646.41};
            cl_double2 B = {500.0, 300.0};
            cl_double2 C = {700.0, 646.41};
            snowflakeTriangleCurves = {Utils::CombineTwoDouble2Vectors(B, A),
                                       Utils::CombineTwoDouble2Vectors(C, B),
                                       Utils::CombineTwoDouble2Vectors(A, C)};
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
                Utils::CombineTwoDouble2Vectors(B, A),
                Utils::CombineTwoDouble2Vectors(A, C),
                Utils::CombineTwoDouble2Vectors(D, B),
                Utils::CombineTwoDouble2Vectors(C, D),
            };
        }
        curve_variants = {{singleCurve, "single curve"},
                          {twoCurvesFace2Face, "two curves"},
                          {snowflakeTriangleCurves, "triangle"},
                          {snowflakeSquareCurves, "square"},
                          {snowflakeSomeFigure, "some figure"}};
    }

    for (int iterations : iteration_count_variants) {
        for (const auto& curve_variant : curve_variants) {
            auto fixture_family = std::make_shared<FixtureFamily>();
            fixture_family->name =
                (boost::format("Koch curve, %1%, %2% iterations, %3%") %
                 OpenClTypeTraits<T>::short_description % iterations % curve_variant.description)
                    .str();
            // TODO fixture_family->element_count can be calculated but is not trivial

            std::vector<T4> casted_curves;
            std::transform(
                curve_variant.curves.cbegin(), curve_variant.curves.cend(),
                std::back_inserter(casted_curves), [](const cl_double4& v) -> T4 {
                    return Utils::StaticCastVector4<T4, cl_double4>(v);
                });

            for (auto& platform : platform_list_.OpenClPlatforms()) {
                for (auto& device : platform->GetDevices()) {
                    fixture_family->fixtures.insert(
                        std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                            FixtureId(fixture_family->name, device, ""),
                            std::make_shared<KochCurveOpenClFixture<T, T4>>(
                                std::dynamic_pointer_cast<OpenClDevice>(device), iterations,
                                casted_curves, 1000.0, 1000.0, fixture_family->name)));
                }
            }
            fixture_families_.push_back(fixture_family);
        }
    }
}

template <typename T, typename P>
std::vector<std::shared_ptr<FixtureFamily>> CreateMultibrotSetFixtures(
    const PlatformList& platform_list) {
    std::vector<double> powers = {1.0, 2.0, 3.0, 7.0, 3.5, 0.1, 0.5};
    std::complex<double> min{-2.5, -2.0};
    std::complex<double> max{1.5, 2.0};
    for (double power : powers) {
        auto fixture_family = std::make_shared<FixtureFamily>();
        fixture_family->name = (boost::format("%1%, %3%, %4%") %
                                ((power == 2.0) ? std::string("Mandelbrot set")
                                                : "Multibrot set, power " + std::to_string(power)) %
                                power % OpenClTypeTraits<T>::short_description %
                                MultibrotResultConstants<P>::pixel_type_description)
                                   .str();
        fixture_family->element_count =
            MultibrotSetParams<T>::width_pix * MultibrotSetParams<T>::height_pix;

        for (auto& platform : platform_list_.OpenClPlatforms()) {
            for (auto& device : platform->GetDevices()) {
                fixture_family->fixtures.insert(
                    std::make_pair<const FixtureId, std::shared_ptr<Fixture>>(
                        FixtureId(fixture_family->name, device, ""),
                        std::make_shared<MultibrotOpenClFixture<T, P>>(
                            std::dynamic_pointer_cast<OpenClDevice>(device),
                            MultibrotSetParams<T>::width_pix, MultibrotSetParams<T>::height_pix,
                            min, max, power, fixture_family->name)));
            }
        }
        fixture_families_.push_back(fixture_family);
    }
}
#endif
