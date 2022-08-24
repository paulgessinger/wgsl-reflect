cmake_policy(SET CMP0048 NEW)
project(wgsl-reflect VERSION 0.1.0)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(cppts)
add_subdirectory(wgsl_reflect)

option("BUILD_TESTS" OFF)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()