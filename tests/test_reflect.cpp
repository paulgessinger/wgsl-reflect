#define CATCH_CONFIG_MAIN

#include "catch2/catch_all.hpp"

#include "wgsl_reflect/reflect.hpp"

#include <tree_sitter_wgsl.h>

#include "cppts/node.hpp"
#include "cppts/parser.hpp"
#include "cppts/query.hpp"
#include "cppts/tree.hpp"

#include "util.hpp"

#include <stdexcept>

TEST_CASE("Reflect construction", "[reflect]") {
  std::string source = load_file("simple.wgsl");
  REQUIRE_NOTHROW(wgsl_reflect::Reflect{source});
  REQUIRE_NOTHROW(wgsl_reflect::Reflect{test_file_path("simple.wgsl")});

  REQUIRE_THROWS_AS(wgsl_reflect::Reflect{std::filesystem::path{"invalid"}},
                    std::ios_base::failure);
}

TEST_CASE("Reflect entrypoints", "[reflect]") {
  wgsl_reflect::Reflect reflect{load_file("simple.wgsl")};

  REQUIRE(reflect.entries().size() == 2);
  REQUIRE(reflect.entry(0).type == wgsl_reflect::EntryType::Vertex);
  REQUIRE(reflect.entry(0).function.name == "vs_main");
  REQUIRE(reflect.entry(1).type == wgsl_reflect::EntryType::Vertex);
  REQUIRE(reflect.entry(1).function.name == "fs_main");
}

TEST_CASE("Parse functions", "[reflect]") {
  cppts::Parser parser{tree_sitter_wgsl()};

  std::string source = R"WGSL(
  fn other(a: int32, b:int32) -> int32 {
      return a + b;
  })WGSL";
  cppts::Tree tree{parser, source};

  wgsl_reflect::Function function{tree.rootNode()};

  REQUIRE(function.name == "other");
}
