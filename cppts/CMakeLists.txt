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

file(GLOB cpptsfiles include/cppts/*.hpp src/cppts/*.cpp)
add_library(cppts STATIC ${cpptsfiles})
set_target_properties(cppts PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(cppts PUBLIC include)
target_link_libraries(cppts PUBLIC tree-sitter)

