include(FetchContent)
FetchContent_Declare(tree-sitter-wgsl
        URL https://github.com/szebniok/tree-sitter-wgsl/archive/272e89e.zip
        URL_HASH SHA256=7d2c69ecbb09c1c6f7b4a6c627c1d7e55cf5f342fdcfd14aff28d4b8d78a33ac
        )
if (NOT tree-sitter-wgsl_POPULATED)
    FetchContent_Populate(tree-sitter-wgsl)
endif ()
add_library(tree-sitter-wgsl STATIC ${tree-sitter-wgsl_SOURCE_DIR}/src/parser.c)
target_include_directories(tree-sitter-wgsl PUBLIC ${tree-sitter-wgsl_SOURCE_DIR}/src ${tree-sitter-wgsl_SOURCE_DIR}/include)

file(MAKE_DIRECTORY ${tree-sitter-wgsl_SOURCE_DIR}/include)
file(WRITE ${tree-sitter-wgsl_SOURCE_DIR}/include/tree_sitter_wgsl.h "
struct TSLanguage;\n\
#ifdef __cplusplus
extern \"C\" {\n\
#endif
extern const TSLanguage *tree_sitter_wgsl(void);\n\
#ifdef __cplusplus
}
#endif
")
