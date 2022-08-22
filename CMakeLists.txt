project(wgsl-reflect)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(tree-sitter
        URL https://github.com/tree-sitter/tree-sitter/archive/refs/tags/v0.20.6.tar.gz
        URL_HASH SHA256=4d37eaef8a402a385998ff9aca3e1043b4a3bba899bceeff27a7178e1165b9de
        )
if (NOT tree-sitter_POPULATED)
    FetchContent_Populate(tree-sitter)
endif ()

set(tsfiles ${tree-sitter_SOURCE_DIR}/lib/src/lib.c)
add_library(tree-sitter STATIC ${tsfiles})
target_include_directories(tree-sitter PUBLIC ${tree-sitter_SOURCE_DIR}/lib/include)
set_property(TARGET tree-sitter PROPERTY C_STANDARD 99)


FetchContent_Declare(tree-sitter-wgsl
        URL https://github.com/szebniok/tree-sitter-wgsl/archive/272e89e.zip
        URL_HASH SHA256=7d2c69ecbb09c1c6f7b4a6c627c1d7e55cf5f342fdcfd14aff28d4b8d78a33ac
        )
if (NOT tree-sitter-wgsl_POPULATED)
    FetchContent_Populate(tree-sitter-wgsl)
endif ()
add_library(tree-sitter-wgsl STATIC ${tree-sitter-wgsl_SOURCE_DIR}/src/parser.c)
target_include_directories(tree-sitter-wgsl PUBLIC ${tree-sitter-wgsl_SOURCE_DIR}/src)

message(STATUS ${CMAKE_CURRENT_SOURCE_DIR}/include/cppts/*.hpp)
file(GLOB cpptsfiles ${CMAKE_CURRENT_SOURCE_DIR}/include/cppts/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/cppts/*.cpp)
add_library(cppts STATIC ${cpptsfiles})
set_target_properties(cppts PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(cppts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cppts PUBLIC tree-sitter)

add_executable(wgsl-reflect-test src/main.cpp)
target_link_libraries(wgsl-reflect-test PUBLIC cppts tree-sitter-wgsl)

