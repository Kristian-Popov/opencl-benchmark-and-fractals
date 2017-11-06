cmake_minimum_required (VERSION 3.1)

project(unit_tests)

add_executable (unit_tests 
	opencl_math_tests.cpp
	koch_curve_tests.cpp
	unit_tests.cpp
)

target_include_directories (unit_tests PUBLIC ${OpenCL_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/contrib 
	${Boost_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/utils )
target_link_libraries (unit_tests ${OpenCL_LIBRARY} ${Boost_LIBRARIES} utils)

if (UNIX)
    target_link_libraries (unit_tests pthread)
endif (UNIX)

set_property(TARGET unit_tests PROPERTY CXX_STANDARD 14)
set_property(TARGET unit_tests PROPERTY CXX_STANDARD_REQUIRED ON)

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
