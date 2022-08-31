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

TEST_CASE("Reflect functions", "[reflect]") {
  wgsl_reflect::Reflect reflect{load_file("simple.wgsl")};

  REQUIRE(reflect.functions().size() == 3);
}

TEST_CASE("Reflect entrypoints", "[reflect]") {
  wgsl_reflect::Reflect reflect{load_file("simple.wgsl")};

  REQUIRE(reflect.entries().vertex.size() == 1);
  REQUIRE(reflect.entries().fragment.size() == 1);
  REQUIRE(reflect.entries().compute.size() == 1);

  REQUIRE(reflect.vertex(0).name == "vs_main");
  REQUIRE(reflect.fragment(0).name == "fs_main");
  REQUIRE(reflect.compute(0).name == "other");
}

TEST_CASE("Parse function", "[reflect]") {
  cppts::Parser parser{tree_sitter_wgsl()};

  SECTION("Multiple inputs") {
    std::string source = R"WGSL(
  fn other(a: i32, b:i32) -> i32 {
      return a + b;
  })WGSL";
    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode()};

    REQUIRE(function.name == "other");
    REQUIRE(function.inputs.size() == 2);
    REQUIRE(function.inputs[0].name == "a");
    REQUIRE(function.inputs[0].type == "i32");
    REQUIRE(function.inputs[1].name == "b");
    REQUIRE(function.inputs[1].type == "i32");
  }

  SECTION("No inputs") {
    std::string source = R"WGSL(
  fn noarg() -> i32 {
      return a + b;
  })WGSL";
    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode()};

    REQUIRE(function.name == "noarg");
    REQUIRE(function.inputs.size() == 0);
  }

  SECTION("Flat inputs with locations") {
    std::string source = R"WGSL(
    @fragment
    fn main(@location(0) in1: i32, @location(2) @interpolate(flat) in2: f32) -> i32 {
        return a + b;
    })WGSL";

    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode()};
    REQUIRE(function.name == "main");
    REQUIRE(function.inputs.size() == 2);

    auto& in1 = function.inputs[0];
    REQUIRE(in1.name == "in1");
    REQUIRE(in1.type == "i32");
    REQUIRE(in1.attributes.size() == 1);
    REQUIRE(in1.attributes[0].name == "location");
    REQUIRE(in1.attributes[0].value == "0");

    auto& in2 = function.inputs[1];
    REQUIRE(in2.name == "in2");
    REQUIRE(in2.type == "f32");
    REQUIRE(in2.attributes.size() == 2);
    REQUIRE(in2.attributes[0].name == "location");
    REQUIRE(in2.attributes[0].value == "2");
    REQUIRE(in2.attributes[1].name == "interpolate");
    REQUIRE(in2.attributes[1].value == "flat");
  }
}

// struct A {
//   @location(0) x: f32,
//                    // Despite locations being 16-bytes, x and y cannot share
//                    a location
//                    @location(1) y: f32
// }
//
//// in1 occupies locations 0 and 1.
//// in2 occupies location 2.
//// The return value occupies location 0.
//@fragment
//    fn fragShader(in1: A, @location(2) in2: f32) -> @location(0) vec4<f32> {
//  // ...
//}
//
