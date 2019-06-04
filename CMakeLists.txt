cmake_minimum_required (VERSION 3.1)

# Set used language
enable_language(CXX)

# Find OpenCL and Boost libraries.
find_package(OpenCL)
# If OpenCL was not found, use headers in contrib folder and ICD Loader library
# (OpenCL.lib or libOpenCL.so)
if(NOT OpenCL_FOUND)
    message( "OpenCL SDK not found, reverting to ICD Loader to use as a library" )
    file( COPY ${CMAKE_SOURCE_DIR}/contrib/OpenCL-Headers/CL
        DESTINATION ${CMAKE_SOURCE_DIR}/contrib/Khronos-OpenCL-ICD-Loader/inc )
    add_subdirectory( contrib/Khronos-OpenCL-ICD-Loader )
    set(OpenCL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/contrib/OpenCL-Headers/)
    set(OpenCL_LIBRARY ${CMAKE_SOURCE_DIR}/contrib/Khronos-OpenCL-ICD-Loader/)
    set(OpenCL_LIBRARIES OpenCL)
endif()

add_definitions( -DCL_TARGET_OPENCL_VERSION=220 )

# Boost should be located in default OS location (often a case on Linux),
# otherwise set BOOST_ROOT variable
# Minimal Boost version is 1.65.1
find_package(Boost 1.65.1 REQUIRED )

# Look for libraries in the following folders
link_directories(${OpenCL_LIBRARY} ${Boost_LIBRARY_DIRS})

# Add nlohmann_json library and disable its unit tests
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders ON CACHE INTERNAL "")
add_subdirectory( contrib/nlohmann-json )

# Add subprojects
add_subdirectory(multibrot_opencl)
add_subdirectory(multibrot_console)
add_subdirectory(utils)
add_subdirectory(unit_tests)
add_subdirectory(contrib/lodepng)

# Add main project
project(OpenCLBenchmarkSuite CXX)

add_library( commonlib STATIC
    coordinate_formatter.cpp
    coordinate_formatter.h
    data_verification_failed_exception.h
    fixture_autoregistrar.h
    fixture_register_macros.h
    fixture_registry.h
    command_line_processor.h
    fixture_runner.h
    half_precision_normal_distribution.h
    mapped_opencl_buffer.h
    opencl_type_traits.h
    run_settings.h

    documents/csv_document.h
    documents/svg_document.cpp
    documents/svg_document.h

    devices/device_interface.h
    devices/opencl_device.h
    devices/opencl_platform.h
    devices/platform_interface.h
    devices/platform_list.h

    fixtures/damped_wave_opencl_fixture.cpp
    fixtures/damped_wave_opencl_fixture.h
    fixtures/fixture.h
    fixtures/fixture_family.h
    fixtures/fixture_id.h
    fixtures/koch_curve_opencl_fixture.h
    fixtures/multibrot_opencl_fixture.cpp
    fixtures/multibrot_opencl_fixture.h
    fixtures/trivial_factorial_opencl_fixture.h

    iterators/sequential_values_iterator.h
    iterators/random_values_iterator.h
    iterators/data_source.h

    indicators/duration_indicator.h

    reporters/benchmark_reporter.h
    reporters/benchmark_results.h
    reporters/json_benchmark_reporter.h
)

target_include_directories( commonlib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${OpenCL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/contrib
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/utils
)
target_link_libraries( commonlib
    ${OpenCL_LIBRARIES}
    ${Boost_LIBRARIES}
    utils
    lodepng
    nlohmann_json::nlohmann_json
    MultibrotOpenCLCalculator
)

if (UNIX)
    target_link_libraries( commonlib pthread)
endif (UNIX)

set_property(TARGET commonlib PROPERTY CXX_STANDARD 14)
set_property(TARGET commonlib PROPERTY CXX_STANDARD_REQUIRED ON)

IF(WIN32) # Check if we are on Windows
  add_definitions(-DWIN32)
  if(MSVC) # Check if we are using the Visual Studio compiler
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    # message(SEND_ERROR "You are using an unsupported Windows compiler! (GCC)")
  else()
    # message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
  endif()
elseif(UNIX)
  add_definitions(-DUNIX)
  # message(SEND_ERROR "You are on an unsupported platform! (Unix)")
else()
  # message(SEND_ERROR "You are on an unsupported platform! (Not Win32 or Unix)")
ENDIF()

# Add executable projects
add_subdirectory(benchmark)
add_subdirectory(run_once_debug)
