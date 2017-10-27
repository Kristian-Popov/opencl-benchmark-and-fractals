cmake_minimum_required (VERSION 3.1)

project(utils)

# Handle OpenCL
find_package(OpenCL REQUIRED)
#include_directories(${OpenCL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/contrib )
#link_directories(${OpenCL_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost/stage/lib)

add_library( utils
	utils.cpp
	operation_step.cpp
	
	utils.h
	operation_step.h
	half_precision_fp.h
)

target_include_directories (utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/boost/ )
target_link_libraries (utils ${OpenCL_LIBRARY})

set_property(TARGET main PROPERTY CXX_STANDARD 14)
set_property(TARGET main PROPERTY CXX_STANDARD_REQUIRED ON)