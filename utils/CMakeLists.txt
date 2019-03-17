cmake_minimum_required (VERSION 3.1)

project(utils)

# Add nlohmann_json library and disable its unit tests
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory( contrib/nlohmann-json )

add_library( utils
    duration.cpp
    utils.cpp
    operation_step.cpp
    program_source_repository.cpp
    program_build_failed_exception.cpp

    duration.h
    utils.h
    operation_step.h
    half_precision_fp.h
    program_source_repository.h
    program_build_failed_exception.h
)

target_include_directories (utils PUBLIC ${OpenCL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/contrib
    ${Boost_INCLUDE_DIRS} )
target_link_libraries (utils PUBLIC ${OpenCL_LIBRARIES} ${Boost_LIBRARIES} nlohmann_json::nlohmann_json )

set_property(TARGET utils PROPERTY CXX_STANDARD 14)
set_property(TARGET utils PROPERTY CXX_STANDARD_REQUIRED ON)
