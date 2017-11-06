cmake_minimum_required (VERSION 3.1)

# Set used language
enable_language(CXX)

# Find OpenCL and Boost libraries.
# It is assumed that Boost can be found either by default location is OS (often a case on Linux) or
# it is located in <project source root>/contrib/boost
# Minimal Boost version is 1.65.1
find_package(OpenCL REQUIRED)
SET( BOOST_ROOT ${CMAKE_SOURCE_DIR}/contrib/boost )
find_package(Boost 1.65.1 REQUIRED )

# Look for libraries in the following folders
link_directories(${OpenCL_LIBRARY} ${Boost_LIBRARY_DIRS})

# Add subprojects
add_subdirectory(utils)
add_subdirectory(unit_tests)

# Add main project
project(OpenCLTestProject)

add_executable (main 
	main.cpp 
	html_document.cpp
	html_benchmark_time_writer.cpp
	stdout_benchmark_time_writer.cpp
	fixture_runner.cpp
	svg_document.cpp
	damped_wave_fixture.inl
	
	trivial_factorial_fixture.h
	damped_wave_fixture.h
	koch_curve_fixture.h

	fixture.h
	fixture_that_returns_data.h
	html_document.h
	html_benchmark_time_writer.h
	csv_document.h
	benchmark_time_writer_interface.h
	stdout_benchmark_time_writer.h
	fixture_runner.h
	sequential_values_iterator.h
	random_values_iterator.h
	svg_document.h
	)

target_include_directories (main PUBLIC ${OpenCL_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/contrib 
	${Boost_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/utils )
target_link_libraries (main ${OpenCL_LIBRARY} ${Boost_LIBRARIES} utils)

if (UNIX)
    target_link_libraries (main pthread)
endif (UNIX)

set_property(TARGET main PROPERTY CXX_STANDARD 14)
set_property(TARGET main PROPERTY CXX_STANDARD_REQUIRED ON)

# Many compilers use implicit promotion of single and double precision floating point values to extended precision ones by default
# This may cause issues with data verification since OpenCL doesn't perform this promotion implicitly
# TODO this definiteliy doesn't work on g++, some variant of "-mfpmath" should be used instead
# On VC++: /fp:strict, but still not sure if it is enough
# target_compile_definitions(main PRIVATE FLT_EVAL_METHOD=0)
IF(WIN32) # Check if we are on Windows
  if(MSVC) # Check if we are using the Visual Studio compiler
	# TODO is INTERFACE property correct?
    #target_compile_definitions(main INTERFACE "/fp:strict")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:strict") # TODO these options doesn't seem to work
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    message(SEND_ERROR "You are using an unsupported Windows compiler! (GCC)")
  else()
    message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
  endif()
elseif(UNIX)
  message(SEND_ERROR "You are on an unsupported platform! (Unix)")
else()
  message(SEND_ERROR "You are on an unsupported platform! (Not Win32 or Unix)")
ENDIF()

if (UNIX)
    # SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fext-numeric-literals" )
endif (UNIX)
