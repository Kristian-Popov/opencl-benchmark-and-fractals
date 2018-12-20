cmake_minimum_required (VERSION 3.1)

project(utils)

add_library( utils
	utils.cpp
	operation_step.cpp
	program_source_repository.cpp
	program_build_failed_exception.cpp
	
	utils.h
	operation_step.h
	half_precision_fp.h
	program_source_repository.h
	koch_curve_utils.h
	program_build_failed_exception.h
)

target_include_directories (utils PUBLIC ${OpenCL_INCLUDE_DIRS} 
	${CMAKE_SOURCE_DIR}/contrib 
	${Boost_INCLUDE_DIRS} )
target_link_libraries (utils ${OpenCL_LIBRARIES} ${Boost_LIBRARIES} )

set_property(TARGET utils PROPERTY CXX_STANDARD 14)
set_property(TARGET utils PROPERTY CXX_STANDARD_REQUIRED ON)
