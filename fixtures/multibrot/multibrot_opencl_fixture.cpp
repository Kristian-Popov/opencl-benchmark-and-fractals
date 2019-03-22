#include "fixtures/multibrot/multibrot_opencl_fixture.h"

#include "lodepng/source/lodepng.h"

#include "utils/utils.h"

namespace
{
    template<typename T>
    struct TempValueConstants {
        static const char* required_extension;
    };

    const char* TempValueConstants<half_float::half>::required_extension = "cl_khr_fp16";
    const char* TempValueConstants<float>::required_extension = "";
    const char* TempValueConstants<double>::required_extension = "cl_khr_fp64";

    template<typename P>
    struct ResultTypeConstants
    {
        static const int max_iterations;
        static const unsigned bitdepth;
    };

    const int ResultTypeConstants<cl_uchar>::max_iterations = 255;
    const unsigned ResultTypeConstants<cl_uchar>::bitdepth = 8;

    const int ResultTypeConstants<cl_ushort>::max_iterations = 10000;
    const unsigned ResultTypeConstants<cl_ushort>::bitdepth = 16;
}

template<typename T, typename P>
MultibrotOpenClFixture<T, P>::MultibrotOpenClFixture(
    const std::shared_ptr<OpenClDevice>& device,
    size_t width_pix, size_t height_pix,
    std::complex<double> input_min,
    std::complex<double> input_max,
    double power,
    const std::string& fixture_name
)
    : device_( device )
    , width_pix_( width_pix )
    , height_pix_( height_pix )
    , input_min_( input_min )
    , input_max_( input_max )
    , power_( power )
    , fixture_name_( fixture_name )
    , output_data_( width_pix * height_pix )
{}

template<typename T, typename P>
void MultibrotOpenClFixture<T, P>::Initialize()
{
    calculator_ = std::make_unique<MultibrotOpenClCalculator<T, P>>(
        device_->device(), device_->GetContext(), width_pix_, height_pix_, power_,
        ResultTypeConstants<cl_uchar>::max_iterations
    );
}

template<typename T, typename P>
std::vector<std::string> MultibrotOpenClFixture<T, P>::GetRequiredExtensions()
{
    std::string requiredExtension = TempValueConstants<T>::required_extension;
    std::vector<std::string> result;
    if( !requiredExtension.empty() )
    {
        result.push_back( requiredExtension );
    }
    return result;
}

template<typename T, typename P>
std::unordered_map<OperationStep, Duration> MultibrotOpenClFixture<T, P>::Execute()
{
    boost::compute::event calc_event;
    auto copy_future = calculator_->Calculate(
        input_min_, input_max_, width_pix_, height_pix_, output_data_.begin(), &calc_event
    );

    copy_future.wait();

    std::unordered_map<OperationStep, boost::compute::event> events;
    events.emplace( OperationStep::Calculate1, calc_event );
    events.emplace( OperationStep::CopyOutputDataFromDevice1, copy_future.get_event() );

    return Utils::GetOpenCLEventDurations( events );
}

template<typename T, typename P>
void MultibrotOpenClFixture<T, P>::StoreResults()
{
    unsigned error = lodepng::encode( fixture_name_ + ".png",
        reinterpret_cast<const unsigned char*>( output_data_.data() ), width_pix_, height_pix_, LCT_GREY,
        ResultTypeConstants<P>::bitdepth );
    if( error )
    {
        throw std::runtime_error( "PNG image build failed" );
    }
}