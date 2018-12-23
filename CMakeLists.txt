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

# Add subprojects
add_subdirectory(utils)
add_subdirectory(unit_tests)
add_subdirectory(contrib/lodepng)

# Add main project
project(OpenCLTestProject)

add_executable (main 
	main.cpp 
	# html_document.cpp
	# html_benchmark_reporter.cpp
	svg_document.cpp

	fixture_runner.h
	fixture_runner.cpp

	# html_document.h
	# html_benchmark_reporter.h
	csv_document.h
	svg_document.h
	kernel_map.h
	document_writers/png_document_builder.h

	devices/device_interface.h
	devices/opencl_device.h
	devices/opencl_platform.h
	devices/platform_interface.h
	devices/platform_list.h

	fixtures/fixture.h
	fixtures/fixture_family.h
	fixtures/fixture_id.h
	fixtures/trivial_factorial_opencl_fixture.h
	# fixtures/koch_curve_fixture.h
	# fixtures/multibrot_fractal_fixture.cpp
	# fixtures/multibrot_fractal_fixture.h
	# fixtures/damped_wave_fixture.inl
	# fixtures/damped_wave_fixture.h
	# fixtures/multiprecision_factorial_fixture.h

	iterators/sequential_values_iterator.h
	iterators/random_values_iterator.h
	iterators/data_source.h

	indicators/duration_indicator.cpp
	indicators/duration_indicator.h
	indicators/indicator_interface.h
	indicators/indicator_serializer.cpp
	indicators/indicator_serializer.h
	indicators/indicator_value_interface.h
	indicators/string_indicator_value.h
	indicators/numeric_indicator_value.h

	reporters/benchmark_results.h
	reporters/json_benchmark_reporter.cpp
	reporters/json_benchmark_reporter.h
	reporters/benchmark_reporter.h
	)

target_include_directories (main PUBLIC ${CMAKE_SOURCE_DIR}
	${OpenCL_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/contrib 
	${Boost_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/utils )
target_link_libraries (main ${OpenCL_LIBRARIES} ${Boost_LIBRARIES} utils lodepng)

if (UNIX)
    target_link_libraries (main pthread)
endif (UNIX)

set_property(TARGET main PROPERTY CXX_STANDARD 14)
set_property(TARGET main PROPERTY CXX_STANDARD_REQUIRED ON)

IF(WIN32) # Check if we are on Windows
  if(MSVC) # Check if we are using the Visual Studio compiler
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    # message(SEND_ERROR "You are using an unsupported Windows compiler! (GCC)")
  else()
    # message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
  endif()
elseif(UNIX)
  # message(SEND_ERROR "You are on an unsupported platform! (Unix)")
else()
  # message(SEND_ERROR "You are on an unsupported platform! (Not Win32 or Unix)")
ENDIF()

if (UNIX)
    # SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fext-numeric-literals" )
endif (UNIX)
