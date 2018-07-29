#pragma once

#include <memory>
#include <random>
#include <stdexcept>
#include <cmath>

#include "utils.h"
#include "fixtures/fixture.h"
#include "data_verification_failed_exception.h"
#include "kernel_map.h"

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>

class MultiprecisionNumber
{
public:
    MultiprecisionNumber( int size, boost::compute::context& context, boost::compute::command_queue& queue )
        : size_( size )
        , deviceBuffer_( context )
        //, deviceBuffer_( static_cast<size_t>( size ), 0ull, queue ) // Fill device buffer with zeroes // TODO by some reason doesn't compile
        , queue_( queue )
        , hostBuffer_( size, 0 )
    {
        EXCEPTION_ASSERT( size > 0 );
        deviceBuffer_.resize( size, queue );
        boost::compute::fill( deviceBuffer_.begin(), deviceBuffer_.end(), 0ull, queue );
    }

    // Set buffer data. Only numbers that fit uint32_t (a single normalized word) can be used at a moment
    // Data are copied to device by the end of this function, but event is returned so operation duration can be retrieved.
    boost::compute::event Set( uint32_t val )
    {
        hostBuffer_.at( 0 ) = val;
        std::fill( hostBuffer_.begin() + 1, hostBuffer_.end(), 0 );
        auto event = boost::compute::copy_async( hostBuffer_.begin(), hostBuffer_.end(), deviceBuffer_.begin(), queue_ ).get_event();
        event.wait();
        return event;
    }

    // Convert data to uint64_t. Returns value only if it fits
    // Data are copied to host by the end of this function, but event is returned so operation duration can be retrieved.
    std::pair<boost::compute::event, boost::optional<uint64_t>> Retrieve()
    {
        auto event = boost::compute::copy_async( deviceBuffer_.begin(), deviceBuffer_.end(), hostBuffer_.begin(), queue_ ).get_event();
        event.wait();
        EXCEPTION_ASSERT( hostBuffer_.size() == size_ );
        // Check if value fits uint64_t
        bool fits = true;
        for( int i = 2; i < size_; ++i )
        {
            if( hostBuffer_.at( i ) > 0 )
            {
                fits = false;
            }
        }
        uint64_t secondWord = ( size_ > 1 ) ? hostBuffer_.at( 1 ) : 0;
        uint64_t result = ( secondWord << 32ull ) + hostBuffer_.at( 0 );
        return std::make_pair( event, fits ? result : boost::optional<uint64_t>() );
    }

    void SetAsKernelArgument( boost::compute::kernel& kernel, size_t index )
    {
        kernel.set_arg( index, deviceBuffer_ );
    }

private:
    bool IsEventCompleted( const boost::compute::event& event ) const
    {
        return ( event.get() != 0 ) && ( event.status() == boost::compute::event::execution_status::complete );
    }

    int size_;
    std::vector<cl_ulong> hostBuffer_;
    boost::compute::vector<cl_ulong> deviceBuffer_;
    boost::compute::command_queue& queue_;
};

/*
    TODO parametrize word type
*/
class MultiprecisionFactorialFixture: public Fixture
{
public:
    MultiprecisionFactorialFixture( cl_uint value,
        const std::string& descriptionSuffix = std::string() )
        : value_( value )
        , descriptionSuffix_( descriptionSuffix )
	{
        EXCEPTION_ASSERT( value_ > 1 );
        /*
            A nice empirical rule to get number of bits needed to store result of n!. If
                n! - factorial of n,
                b(a) - bits needed to store number a, then
                b(n!) = b(n) + b((n-1)!)
        */
        double bitsNeeded = 1.0;
        for( uint32_t val = 2; val <= value_; ++val )
        {
            bitsNeeded += ceil( log2( val ) );
        }
        double wordsNeeded = ceil( bitsNeeded / 32.0 ); // Size is in words, every word holds 32 bits
        EXCEPTION_ASSERT( wordsNeeded < INT_MAX );
        size_ = static_cast<cl_int>( wordsNeeded );
	}

    virtual void Initialize() override
    {
    }

    virtual void InitializeForContext( boost::compute::context& context ) override
    {
        auto queue = std::make_shared<boost::compute::command_queue>(
            context, context.get_device(), boost::compute::command_queue::enable_profiling );
        auto kernel = std::make_shared<KernelMap>( std::vector<std::string>( {
            "MultiprecisionMultWithSingleWord",
            "MultiprecisionNormalize"
            } ),
            // TODO remove -w everywhere (it inhibits all warnings)
            context, ProgramSourceRepository::GetMultiprecisionMathSource(), "-Werror"
        );
        auto result = std::make_shared<MultiprecisionNumber>( size_, context, *queue );
        auto errors = std::make_shared<boost::compute::vector<cl_uint>>( 1, context );

        contextSpecificData_.emplace( std::make_pair( context.get(), ContextSpecificData(
            queue,
            kernel,
            result,
            errors
        ) ) );
    }

    virtual std::vector<OperationStep> GetSteps() override
    {
        return {
            OperationStep::CopyInputDataToDevice,
            OperationStep::MultiprecisionMultiplyWords,
            OperationStep::MultiprecisionNormalize,
            OperationStep::CopyErrors,
            OperationStep::CopyOutputDataFromDevice
        };
    }

    virtual std::vector<std::string> GetRequiredExtensions() override
    {
        return { "cl_khr_global_int32_extended_atomics" };
    }

    std::unordered_map<OperationStep, Duration> Execute( boost::compute::context& context ) override
	{
        EXCEPTION_ASSERT( context.get_devices().size() == 1 );
        boost::compute::command_queue& queue = *( contextSpecificData_.at( context ).queue_ );

        std::unordered_multimap<OperationStep, boost::compute::event> events;

        MultiprecisionNumber& outputData = *( contextSpecificData_.at( context ).result_ );
        events.insert( { OperationStep::CopyInputDataToDevice, outputData.Set( 1 ) } );

        boost::compute::vector<cl_uint>& errors = *( contextSpecificData_.at( context ).errors_ );
        boost::compute::fill( errors.begin(), errors.end(), 0ull, queue );

        for( cl_uint param = 2; param <= value_; ++param )
        {
            boost::compute::event lastEvent;
            {
                boost::compute::kernel& kernel = contextSpecificData_.at( context ).kernels_->Get(
                    "MultiprecisionMultWithSingleWord" );

                outputData.SetAsKernelArgument( kernel, 0 );
                kernel.set_arg( 1, size_ );
                kernel.set_arg( 2, param );
                outputData.SetAsKernelArgument( kernel, 3 );
                kernel.set_arg( 4, size_ );
                kernel.set_arg( 5, errors );

                size_t localWorkGroupSize = 0;
                events.insert( { OperationStep::MultiprecisionMultiplyWords,
                    queue.enqueue_1d_range_kernel( kernel, 0, size_, localWorkGroupSize )} );

                std::vector<cl_uint> errorsOnHost( 1, 0 );
                lastEvent = boost::compute::copy_async( errors.begin(), errors.end(), errorsOnHost.begin(), queue ).get_event();
                lastEvent.wait();
                EXCEPTION_ASSERT( errorsOnHost.at( 0 ) == 0 );
                events.insert( { OperationStep::CopyErrors, lastEvent } );
            }

            {
                boost::compute::kernel& kernel = contextSpecificData_.at( context ).kernels_->Get(
                    "MultiprecisionNormalize" );

                outputData.SetAsKernelArgument( kernel, 0 );
                kernel.set_arg( 1, size_ );
                kernel.set_arg( 2, errors );

                events.insert( { OperationStep::MultiprecisionNormalize, queue.enqueue_task( kernel )} );

                std::vector<cl_uint> errorsOnHost( 1, 0 );
                lastEvent = boost::compute::copy_async( errors.begin(), errors.end(), errorsOnHost.begin(), queue ).get_event();
                lastEvent.wait();
                EXCEPTION_ASSERT( errorsOnHost.at( 0 ) == 0 );
                events.insert( {OperationStep::CopyErrors, lastEvent} );
            }
        }

        auto retrieveResults = outputData.Retrieve();
        contextSpecificData_.at( context ).calculatedResult_ = retrieveResults.second;
        events.insert( { OperationStep::CopyOutputDataFromDevice, retrieveResults.first } );

        return Utils::CalculateTotalStepDurations<Duration>( events );
	}

    std::string Description() override
    {
        std::string result = "Multiprecision factorial of " + std::to_string( value_ );
        if (!descriptionSuffix_.empty())
        {
            result += ", " + descriptionSuffix_;
        }
        return result;
    }

    virtual void VerifyResults( boost::compute::context& context ) override
    {
        if( value_ <= 20 )
        {
            try
            {
                auto iter = correctFactorialValues_.find( value_ );
                EXCEPTION_ASSERT( contextSpecificData_.at( context ).calculatedResult_ );
                uint64_t result = contextSpecificData_.at( context ).calculatedResult_.get();
                if( iter != correctFactorialValues_.cend() && result != iter->second )
                {
                    cl_ulong maxAbsError = result - iter->second;
                    throw DataVerificationFailedException( ( boost::format(
                        "Result verification has failed for fixture \"%1%\". "
                        "Maximum absolute error is %2% for input value %3% "
                        "(exact equality is expected)." ) %
                        Description() % maxAbsError % value_ ).str() );
                }
            }
            catch( ... )
            {
                throw;
            }
        }
    }

    virtual ~MultiprecisionFactorialFixture()
    {
    }
private:
    cl_int size_;
    static const std::unordered_map<int, cl_ulong> MultiprecisionFactorialFixture::correctFactorialValues_;
    cl_uint value_;
    std::string descriptionSuffix_;
    struct ContextSpecificData
    {
        std::shared_ptr<boost::compute::command_queue> queue_;
        std::shared_ptr<KernelMap> kernels_;
        std::shared_ptr<MultiprecisionNumber> result_;
        std::shared_ptr<boost::compute::vector<cl_uint>> errors_;
        boost::optional<uint64_t> calculatedResult_;

        ContextSpecificData( std::shared_ptr<boost::compute::command_queue> queue,
            std::shared_ptr<KernelMap> kernels,
            std::shared_ptr<MultiprecisionNumber> result,
            std::shared_ptr<boost::compute::vector<cl_uint>> errors )
            : queue_( std::move( queue ) )
            , kernels_( std::move( kernels ) )
            , result_( std::move( result ) )
            , errors_( std::move( errors ) )
        {}
    };
    std::unordered_map<cl_context, ContextSpecificData> contextSpecificData_;
};

const std::unordered_map<int, cl_ulong> MultiprecisionFactorialFixture::correctFactorialValues_ = {
    {0, 1ull},
    {1,  1ull},
    {2,  2ull},
    {3,  6ull},
    {4,  24ull},
    {5,  120ull},
    {6,  720ull},
    {7,  5040ull},
    {8,  40320ull},
    {9,  362880ull},
    {10,  3628800ull},
    {11,  39916800ull},
    {12,  479001600ull},
    {13,  6227020800ull},
    {14,  87178291200ull},
    {15,  1307674368000ull},
    {16,  20922789888000ull},
    {17,  355687428096000ull},
    {18,  6402373705728000ull},
    {19,  121645100408832000ull},
    {20,  2432902008176640000ull},
};
