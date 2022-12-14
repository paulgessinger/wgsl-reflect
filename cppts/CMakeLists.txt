include(FetchContent)

FetchContent_Declare(tree-sitter
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/tree-sitter/tree-sitter/archive/refs/tags/v0.20.6.tar.gz
        URL_HASH SHA256=4d37eaef8a402a385998ff9aca3e1043b4a3bba899bceeff27a7178e1165b9de
        )
if (NOT tree-sitter_POPULATED)
    FetchContent_Populate(tree-sitter)
endif ()

if (MSVC)
    set(old_flags "${CMAKE_C_FLAGS_INIT}")
    string(REGEX REPLACE "/W[3|4]" "/w" CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT}")
    string(REGEX REPLACE "/W[3|4]" "/w" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif ()

set(tsfiles ${tree-sitter_SOURCE_DIR}/lib/src/lib.c)
add_library(tree-sitter STATIC ${tsfiles})
target_include_directories(tree-sitter PUBLIC ${tree-sitter_SOURCE_DIR}/lib/include)
set_property(TARGET tree-sitter PROPERTY C_STANDARD 99)

FetchContent_Declare(range-v3
        GIT_REPOSITORY https://github.com/ericniebler/range-v3.git
        GIT_TAG 0.12.0)

FetchContent_MakeAvailable(range-v3)

if (MSVC)
    set(CMAKE_C_FLAGS_INIT "${old_flags}")
endif ()

file(GLOB cpptsfiles include/cppts/*.hpp src/cppts/*.cpp)
add_library(cppts STATIC ${cpptsfiles})
set_target_properties(cppts PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(cppts PUBLIC include)
target_link_libraries(cppts PUBLIC tree-sitter range-v3::range-v3)

add_library(cppts::cppts ALIAS cppts)