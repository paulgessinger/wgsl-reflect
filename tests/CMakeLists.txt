if (USE_SYSTEM_CATCH2)
    find_package(Catch2 CONFIG REQUIRED)
else ()
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.0.1 # or a later release
    )

    FetchContent_MakeAvailable(Catch2)

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif ()

include(CTest)
include(Catch)

function(_add_test name file)
    add_executable(${name} ${file})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain)
    target_link_libraries(${name} PUBLIC cppts tree-sitter-wgsl wgsl_reflect::wgsl_reflect)
    target_compile_definitions(${name} PRIVATE TEST_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")
    catch_discover_tests(${name})
endfunction()

_add_test(test_parser test_parser.cpp)
_add_test(test_reflect_parse test_reflect_parse.cpp)
_add_test(test_reflect test_reflect.cpp)
_add_test(test_json test_json.cpp)
