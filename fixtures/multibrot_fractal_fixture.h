#pragma once

#include "fixture_that_returns_data.h"
#include "boost/compute.hpp"

/*
T should be a floating point type (e.g. float, double or half),
*/
template<typename T>
/* FixtureThatReturnsData is used to retrieve data. If we have something like
FixtureThatReturnsData<T>, then we may have trouble writing multiple values into CSV/SVG/etc.
Instead we are using long double that is well suiting to store floating point values
since it can hold any half/float/double value without any precision loss
*/
class MultibrotFractalFixture : public FixtureThatReturnsData<cl_ushort4>
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

    virtual std::vector<std::vector<cl_ushort4>> GetResults() override
    {
        // TODO add more elegant retrieving results for building PNG
        //return outputData_;
        return { outputData_ };
    }

    virtual boost::optional<size_t> GetElementsCount() override
    {
        return GetElementsCountInternal();
    }

    virtual ~MultibrotFractalFixture()
    {
    }
private:
    //std::vector<std::vector<cl_ushort4>> outputData_;
    std::vector<cl_ushort4> outputData_;
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
};

// Forward declarations of possible variations of this class so we can move implementation
// to a source file
template class MultibrotFractalFixture<cl_float>;

template class MultibrotFractalFixture<cl_double>;
