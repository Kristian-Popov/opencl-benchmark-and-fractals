cmake_minimum_required (VERSION 3.1)

project(OpenCLTestProject)

# Handle OpenCL
find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})
link_directories(${OpenCL_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost/stage/lib)

add_executable (main 
	main.cpp 
	utils.cpp
	html_document.cpp
	html_benchmark_time_writer.cpp
	stdout_benchmark_time_writer.cpp
	operation_step.cpp
	fixture_runner.cpp
	
	utils.h
	fixture.h
	fixture_that_returns_data.h
	trivial_factorial_fixture.h 
	operation_step.h
	html_document.h
	html_benchmark_time_writer.h
	damped_wave_fixture.h
	csv_document.h
	benchmark_time_writer_interface.h
	stdout_benchmark_time_writer.h
	fixture_runner.h
	sequential_values_iterator.h
	random_values_iterator.h
	)

# target_include_directories (main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/nonius/include )
target_include_directories (main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost/ )
target_link_libraries (main ${OpenCL_LIBRARY})

# Couldn't get this working on Windows
#SET( BOOST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost )
#SET( BOOST_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost_1_65_1/boost )
#find_package(Boost 1.65.1 REQUIRED COMPONENTS compute chrono )
#target_link_libraries(foo Boost::chrono )

#add_definitions(-D__CL_ENABLE_EXCEPTIONS)

if (UNIX)
    target_link_libraries (main pthread)
endif (UNIX)

set_property(TARGET main PROPERTY CXX_STANDARD 14)
set_property(TARGET main PROPERTY CXX_STANDARD_REQUIRED ON)

# Many compilers use implicit promotion of single and double precision floating point values to extended precision ones by default
# This may cause issues with data verification since OpenCL doesn't perform this promotion implicitly
# TODO investigate if it works with particular compilers (e.g. VC++ 2015 by default uses /fp:precise option that may override this one)
target_compile_definitions(main PRIVATE FLT_EVAL_METHOD=0)

if (UNIX)
    # SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fext-numeric-literals" )
endif (UNIX)
