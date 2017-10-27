cmake_minimum_required (VERSION 3.1)

project(unit_tests)

# Handle OpenCL
find_package(OpenCL REQUIRED)
#include_directories(${OpenCL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/contrib )
#link_directories(${OpenCL_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost/stage/lib)

add_executable (unit_tests 
	opencl_math_tests.cpp
	
	unit_tests_include.h
)

# target_include_directories (main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/nonius/include )
target_include_directories (unit_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost/ utils)
target_link_libraries (unit_tests ${OpenCL_LIBRARY} utils)

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
