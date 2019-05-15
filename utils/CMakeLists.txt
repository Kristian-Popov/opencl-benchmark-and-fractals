cmake_minimum_required (VERSION 3.1)

project(utils)

add_library( utils
    duration.cpp
    utils.cpp
    program_source_repository.cpp
    program_build_failed_exception.cpp

    duration.h
    utils.h
    half_precision_fp.h
    program_source_repository.h
    program_build_failed_exception.h
)

target_include_directories (utils PUBLIC
    ${OpenCL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/contrib
    ${Boost_INCLUDE_DIRS}
)
# We don't really need linking, but this procedure sets all include directories that we need
target_link_libraries (utils PUBLIC nlohmann_json::nlohmann_json )

set_property(TARGET utils PROPERTY CXX_STANDARD 14)
set_property(TARGET utils PROPERTY CXX_STANDARD_REQUIRED ON)
