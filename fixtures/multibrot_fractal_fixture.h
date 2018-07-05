#pragma once

#include "fixtures/fixture.h"
#include "boost/compute.hpp"
#include "half_precision_fp.h"

/*
T should be a floating point type (e.g. float, double or half),
*/
template<typename T>
class MultibrotFractalFixture : public Fixture
{
public:
    explicit MultibrotFractalFixture(
        size_t width, size_t height,
        T realMin, T realMax,
        T imgMin, T imgMax,
        T power,
        const std::string& descriptionSuffix = std::string() );

    //virtual void Initialize() override;

    virtual void InitializeForContext( boost::compute::context& context ) override;

    virtual std::vector<OperationStep> GetSteps() override
    {
        return{
            OperationStep::Calculate,
            OperationStep::MapOutputData,
            OperationStep::UnmapOutputData
        };
    }

    virtual std::vector<std::string> GetRequiredExtensions() override;

    std::unordered_map<OperationStep, Duration> Execute( boost::compute::context& context ) override;

    std::string Description() override;

    static size_t MultibrotFractalFixture<T>::CalcMaxSizeInPix( T min, T max );

    virtual ~MultibrotFractalFixture()
    {
    }
private:
    std::vector<cl_ushort> outputData_;
    T realMin_;
    T realMax_;
    T imgMin_;
    T imgMax_;
    T power_;
    size_t width_;
    size_t height_;
    std::string descriptionSuffix_;
    std::unordered_map<cl_context, boost::compute::kernel> kernels_;

    size_t GetElementsCountInternal() const
    {
        return width_*height_;
    }

    std::vector<std::vector<cl_ushort>> GetResults()
    {
        // TODO add more elegant retrieving results for building PNG
        //return outputData_;
        return {outputData_};
    }

    boost::optional<size_t> GetElementsCount() override
    {
        return GetElementsCountInternal();
    }

    void WriteResults();
};

// Forward declarations of possible variations of this class so we can move implementation
// to a source file
template class MultibrotFractalFixture<cl_float>;

template class MultibrotFractalFixture<cl_double>;

template class MultibrotFractalFixture<half_float::half>;
