#pragma once

#include "fixture.h"

#pragma pack(push, 1)
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
#pragma pack(pop)

template<typename T, typename I>
class DampedWaveFixture : public Fixture
{
public:
    DampedWaveFixture( const std::vector<DampedWaveFixtureParameters<T>>& params,
        const I& inputDataIter, size_t dataSize,
        const std::string& descriptionSuffix = std::string() );

    virtual void Initialize() override;

    virtual void InitializeForContext( boost::compute::context& context ) override;

    virtual std::vector<OperationStep> GetSteps() override;

    virtual std::vector<std::string> GetRequiredExtensions() override;

    std::unordered_map<OperationStep, Duration> Execute( boost::compute::context& context ) override;

    std::string Description() override;

    void VerifyResults();

    virtual boost::optional<size_t> GetElementsCount() override
    {
        return dataSize_;
    }

    virtual void WriteResults() override
    {
        CSVDocument csvDocument( ( Description() + ".csv" ).c_str() );
        csvDocument.AddValues( GetResults() );
        csvDocument.BuildAndWriteToDisk();
    }

    virtual ~DampedWaveFixture()
    {
    }
private:
    typedef DampedWaveFixtureParameters<T> Parameters;

    std::vector<T> inputData_;
    std::vector<T> expectedOutputData_;
    std::vector<T> outputData_;
    std::vector<Parameters> params_;
    I inputDataIter_;
    size_t dataSize_;
    std::string descriptionSuffix_;
    std::unordered_map<cl_context, boost::compute::kernel> kernels_;

    void GenerateData();

    T CalcExpectedValue( T x ) const;

    /*
    Return results in the following form:
    {
    { xmin, y_expected_1, y_1 },
    { x2, y_expected_2, y_2 },
    ...
    { xmax, y_expected_n, y_n },
    }
    First column contains input value, second one - expected value (as calculated on a host CPU),
    third one - value calculated by OpenCL device
    */
    std::vector<std::vector<long double>> GetResults();
};

#include "damped_wave_fixture.inl"