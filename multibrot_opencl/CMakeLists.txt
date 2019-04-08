cmake_minimum_required (VERSION 3.1)

project(MultibrotOpenCLCalculator)

add_library( MultibrotOpenCLCalculator
    multibrot_opencl_calculator.cpp
    multibrot_opencl_calculator.h

    multibrot_parallel_calculator.cpp
    multibrot_parallel_calculator.h

    image_partitioner.h
)

target_include_directories (MultibrotOpenCLCalculator PUBLIC ${OpenCL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/contrib
    # ${CMAKE_SOURCE_DIR}/utils/contrib/nlohmann-json/include
    ${Boost_INCLUDE_DIRS} )
# target_link_libraries (MultibrotOpenCLCalculator PUBLIC ${OpenCL_LIBRARIES} ${Boost_LIBRARIES} )
target_link_libraries (MultibrotOpenCLCalculator PUBLIC nlohmann_json::nlohmann_json )

set_property(TARGET MultibrotOpenCLCalculator PROPERTY CXX_STANDARD 14)
set_property(TARGET MultibrotOpenCLCalculator PROPERTY CXX_STANDARD_REQUIRED ON)
