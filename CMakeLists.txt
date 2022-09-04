cmake_policy(SET CMP0048 NEW)
project(wgsl-reflect VERSION 0.1.0)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)

option("USE_SYSTEM_JSON" OFF)
option("USE_SYSTEM_CLI11" OFF)
option("USE_SYSTEM_CATCH2" OFF)

if (USE_SYSTEM_JSON)
    find_package(nlohmann_json CONFIG REQUIRED)
else ()
    include(FetchContent)
    FetchContent_Declare(json
            DOWNLOAD_EXTRACT_TIMESTAMP true
            URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
            URL_HASH SHA256=8c4b26bf4b422252e13f332bc5e388ec0ab5c3443d24399acb675e68278d341f)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(json)
endif ()

if (USE_SYSTEM_CLI11)
    find_package(CLI11 CONFIG REQUIRED)
else ()
    include(FetchContent)
    FetchContent_Declare(
            cli11
            GIT_REPOSITORY https://github.com/CLIUtils/CLI11
            GIT_TAG v2.2.0
    )
    FetchContent_MakeAvailable(cli11)
endif ()

add_subdirectory(cppts)
add_subdirectory(wgsl_reflect)

option("BUILD_TESTS" OFF)


add_executable(wgsl_reflect_exe wgsl_reflect/src/main.cpp)
set_target_properties(wgsl_reflect_exe PROPERTIES
        OUTPUT_NAME "wgsl_reflect"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(wgsl_reflect_exe PRIVATE nlohmann_json::nlohmann_json CLI11::CLI11 wgsl_reflect::wgsl_reflect)


if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()