#pragma once

#include "boost/compute.hpp"
#include "devices/opencl_device.h"
#include "fixtures/fixture.h"
#include "iterators/data_source_adaptor.h"

#include "half_precision_fp.h"

// TODO get rid of this - non-portable
//#pragma pack(push, 1)
template<typename T>
struct DampedWaveFixtureParameters
{
    T amplitude = 0;
    T dampingRatio = 0;
    T angularFrequency = 0;
    T phase = 0;
    T shift = 0;

    DampedWaveFixtureParameters( T amplitude_, T dampingRatio_, T angularFrequency_, T phase_, T shift_ )
        : amplitude( amplitude_ )
        , dampingRatio( dampingRatio_ )
        , angularFrequency( angularFrequency_ )
        , phase( phase_ )
        , shift( shift_ )
    {
    }
};
//#pragma pack(pop)

/*
paramSet.amplitude * exp(-paramSet.dampingRatio * t) *
cos(paramSet.angularFrequency * t + paramSet.phase);
    Damped wave fixture
    ( A1 * exp(-D1 * t) * cos( F1 * t + P1 ) ) + ...
    Note that this fixture doesn't verify results since we may get very different
    results in edge value between CPU and OpenCL implementation (it's even
    compiler dependent).
    TODO add absolute/relative difference indicators to calculate
    max deviation.
*/
template<typename T>
class DampedWaveOpenClFixture : public Fixture
{
public:
    DampedWaveOpenClFixture(
        const std::shared_ptr<OpenClDevice>& device,
        const std::vector<DampedWaveFixtureParameters<T>>& params,
        const std::shared_ptr<DataSource<T>>& input_data_source,
        size_t dataSize,
        const std::string& fixture_name
    );

    void Initialize() override;

    std::vector<std::string> GetRequiredExtensions() override;

    std::unordered_map<OperationStep, Duration> Execute() override;

    void StoreResults() override;

    std::shared_ptr<DeviceInterface> Device() override
    {
        return device_;
    }
private:
    typedef DampedWaveFixtureParameters<T> Parameters;

    const std::shared_ptr<OpenClDevice> device_;
    std::vector<T> inputData_;
    std::vector<T> outputData_;
    std::vector<Parameters> params_;
    std::shared_ptr<DataSource<T>> input_data_source_;
    size_t dataSize_;
    boost::compute::kernel kernel_;
    std::string fixture_name_;

    void GenerateData();
#if 0
    T CalcExpectedValue( T x ) const;
#endif
    /*
    Return results in the following form:
    {
    { xmin, y_1 },
    { x2, y_2 },
    ...
    { xmax, y_n },
    }
    First column contains input value,
    second one - value calculated by OpenCL device
    */
    std::vector<std::vector<T>> GetResults();
};

// Forward declarations of possible variations of this class so we can move implementation
// to a source file
template class DampedWaveOpenClFixture<float>;
template class DampedWaveOpenClFixture<double>;
template class DampedWaveOpenClFixture<half_float::half>;