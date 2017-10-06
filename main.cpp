#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "trivial_factorial_fixture.h"
#include "operation_id.h"
#include "benchmark_fixture_html_builder.h"
#include "html_document.h"

#include "boost/compute/compute.hpp"

#if 0
 int main(void)
 {
    cl_int err = CL_SUCCESS;
    try {
      std::vector<cl::Platform> platforms;
      cl::Platform::get(&platforms);
      if (platforms.empty()) {
          std::cout << "No OpenCL platforms found" << std::endl;
          WaitForUserInput();
          return EXIT_FAILURE;
      }
      for (cl::Platform& platform: platforms)
      {
          cl_context_properties properties[] =
             { CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(), 0};
          cl::Context context(CL_DEVICE_TYPE_ALL, properties);

          std::string source = ReadFile("../kernel.cl");
          std::cout << "Kernel: " << source << std::endl;

          std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
          cl::Program program = cl::Program(context, source);
          program.build(devices); // TODO enable OpenCL compiler warnings
          std::cout << "Kernel built successfully" << std::endl;

          for (cl::Device& device: devices)
          {
            std::cout << "Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

              cl::Kernel kernel(program, "Factorial", &err);

              int dataSize = 21;

              std::vector<cl_ulong> inputData(dataSize, 0ull);
              cl_ulong counter = 0;
              std::generate(inputData.begin(), inputData.end(), [&counter] ()
              {
                return counter++;
              });
              //cl::Buffer inputBuffer(context, inputData.begin(), inputData.end(), true, true);
              cl::Buffer inputBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, dataSize * sizeof(cl_ulong), &inputData[0]);

              std::vector<cl_ulong> outputData(dataSize, 0ull);
              // LOL processor OpenCL implementation ignores readonly flag in this config
              //cl::Buffer outputBuffer(context, outputData.begin(), outputData.end(), false, true);
              //cl::Buffer outputBuffer(context, outputData.begin(), outputData.end(), false, true);
              cl::Buffer outputBuffer(context, CL_MEM_WRITE_ONLY|CL_MEM_USE_HOST_PTR, dataSize * sizeof(cl_ulong), &outputData[0]);

              kernel.setArg(0, inputBuffer);
              kernel.setArg(1, outputBuffer);

              cl::Event event;
              std::vector<cl::Event> events({event});
              cl::CommandQueue queue(context, device, 0, &err);
              queue.enqueueNDRangeKernel(
                  kernel,
                  cl::NullRange,
                  cl::NDRange(dataSize),
                  cl::NullRange,
                  NULL,
                  &event);
              queue.enqueueReadBuffer(
                  outputBuffer,
                  false,
                  0,
                  dataSize * sizeof(cl_ulong),
                  &outputData[0]
                  //&events ); // TODO why don't we need to wait until event finishes?
                  );

              queue.finish();

              std::cout << "Printing value of buffers:" << std::endl;
              for (int i = 0; i < dataSize; ++i)
              {
                std::cout << inputData.at(i) << "\t" << outputData.at(i) << std::endl;
              }

              std::cout << "Kernel finished" << std::endl <<
                    "--------------------------------------" << std::endl;
            }
      }

      std::cout << "Done" << std::endl;
    }
    catch (cl::Error err) {
       std::cerr
          << "ERROR: "
          << err.what()
          << "("
          << err.err()
          << ")"
          << std::endl;
    }
    
    WaitForUserInput();
   return EXIT_SUCCESS;
 }
#endif

int main( int argc, char** argv )
{
    std::vector<std::unique_ptr<Fixture>> fixtures;
    const std::vector<int> dataSizesForTrivialFactorial = {100, 1000, 100000, 1000000, 100000000};
    std::transform( dataSizesForTrivialFactorial.begin(), dataSizesForTrivialFactorial.end(), std::back_inserter(fixtures),
        [] (int dataSize)
        {
            return std::make_unique<TrivialFactorialFixture>(dataSize);
        } );

    const std::unordered_map<OperationId, std::string> operationDescriptions = {
        std::make_pair( OperationId::CopyInputDataToDevice, "Copy input data to device" ),
        std::make_pair( OperationId::Calculate, "Calculation" ),
        std::make_pair( OperationId::CopyOutputDataFromDevice, "Copy output data from device" ),
    };

    typedef double OutputNumericType;
    typedef std::chrono::duration<OutputNumericType, std::micro> OutputDurationType;
    auto targetFixtureExecutionTime = std::chrono::milliseconds(1); // Time how long fixture should execute, if larger than execution time than few iterations are done
    const char* outputHTMLFileName = "output.html";
    BenchmarkFixtureHTMLBuilder htmlDocumentBuilder( outputHTMLFileName );

    std::vector<boost::compute::platform> platforms = boost::compute::system::platforms();
    for (std::unique_ptr<Fixture>& fixture: fixtures)
    {
        fixture->Init();
        BenchmarkFixtureHTMLBuilder::BenchmarkFixtureResultForFixture dataForHTMLBuilder;
        dataForHTMLBuilder.fixtureName = fixture->Description();

        for( boost::compute::platform& platform: platforms )
        {
            BenchmarkFixtureHTMLBuilder::BenchmarkFixtureResultForPlatform perPlatformResults;
            perPlatformResults.platformName = platform.name();

            std::vector<boost::compute::device> devices = platform.devices();
            for( boost::compute::device& device: devices )
            {
                BenchmarkFixtureHTMLBuilder::BenchmarkFixtureResultForDevice perDeviceResults;
                perDeviceResults.deviceName = device.name();

                try
                {
                    std::cout << "\"" << platform.name() << "\", \"" << device.name() << "\", \"" << fixture->Description() << "\":" << std::endl;

                    boost::compute::context context( device );

                    std::vector<std::unordered_map<OperationId, Fixture::ExecutionResult>> results;

                    // Warm-up for one iteration to get estimation of execution time
                    std::unordered_map<OperationId, Fixture::ExecutionResult> warmupResult = fixture->Execute( context );
                    OutputDurationType totalOperationDuration = std::accumulate( warmupResult.begin(), warmupResult.end(), OutputDurationType::zero(),
                        [] ( OutputDurationType acc, const std::pair<OperationId, Fixture::ExecutionResult>& r )
                        {
                            return acc + r.second.duration;
                        } );
                    int iterationCount = static_cast<int>(std::ceil(targetFixtureExecutionTime / totalOperationDuration));
                    EXCEPTION_ASSERT(iterationCount >= 1);
                    for( int i = 0; i < iterationCount; ++i)
                    {
                        results.push_back(fixture->Execute( context ) );
                    }

                    for( OperationId id : OperationIdList::Build() )
                    {
                        std::vector<OutputDurationType> perOperationResults;
                        std::transform( results.begin(), results.end(), std::back_inserter( perOperationResults ),
                            [id]( const std::unordered_map<OperationId, Fixture::ExecutionResult>& d )
                        {
                            const Fixture::ExecutionResult& r = d.at( id );
                            EXCEPTION_ASSERT( r.operationId == id );
                            return std::chrono::duration_cast<OutputDurationType>( r.duration );
                        } );

                        //TODO "accumulate" can safely be changed to "reduce" here to increase performance
                        OutputDurationType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), OutputDurationType::zero() ) / perOperationResults.size();

                        perDeviceResults.perOperationResults.insert( { id, avg } );
                    }

                    for ( OperationId id: OperationIdList::Build())
                    {
                        std::vector<double> perOperationResults;
                        std::transform(results.begin(), results.end(), std::back_inserter( perOperationResults ),
                            [id] (const std::unordered_map<OperationId, Fixture::ExecutionResult>& d )
                            {
                                const Fixture::ExecutionResult& r = d.at(id);
                                EXCEPTION_ASSERT( r.operationId == id );
                                return std::chrono::duration_cast<OutputDurationType>(r.duration).count();
                            } );

                        auto minmax = std::minmax_element( perOperationResults.begin(), perOperationResults.end() );
                        OutputNumericType min = *minmax.first;
                        OutputNumericType max = *minmax.second;

                        //TODO "accumulate" can safely be changed to "reduce" here to increase performance
                        OutputNumericType avg = std::accumulate( perOperationResults.begin(), perOperationResults.end(), 0.0) / perOperationResults.size();

                        std::cout << "\t\"" << operationDescriptions.at(id) << "\": " << min << " - " << max << " (avg " << avg << ") microseconds" << std::endl;
                    }

                    std::cout << std::endl;
                }
                catch(boost::compute::opencl_error& e)
                {
                    std::cout << "OpenCL error occured: " << e.what() << std::endl;
                }

                perPlatformResults.perDeviceResults.push_back(perDeviceResults);
            }

            dataForHTMLBuilder.perFixtureResults.push_back(perPlatformResults);
        }
        htmlDocumentBuilder.AddFixtureResults( dataForHTMLBuilder );

        // Destroy fixture to release some memory sooner
        fixture.reset();
    }
    htmlDocumentBuilder.GetHTMLDocument()->BuildAndWriteToDisk();

    return 0;
}

