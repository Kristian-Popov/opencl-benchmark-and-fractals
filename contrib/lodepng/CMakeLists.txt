cmake_minimum_required (VERSION 3.1)

# This CMake file includes only library itself.
#project(utils)

add_library( lodepng
	source/lodepng.cpp
	
	source/lodepng.h
)

target_include_directories (lodepng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/source)