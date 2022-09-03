#include "catch2/catch_all.hpp"

#include "wgsl_reflect/reflect.hpp"

#include <tree_sitter_wgsl.h>

#include "cppts/node.hpp"
#include "cppts/parser.hpp"
#include "cppts/query.hpp"
#include "cppts/tree.hpp"

#include "util.hpp"

#include <stdexcept>

using namespace std::string_literals;

TEST_CASE("Reflect construction", "[reflect]") {
  std::string source = load_file("simple.wgsl");
  REQUIRE_NOTHROW(wgsl_reflect::Reflect{source});
  REQUIRE_NOTHROW(wgsl_reflect::Reflect{test_file_path("simple.wgsl")});

  REQUIRE_THROWS_AS(wgsl_reflect::Reflect{std::filesystem::path{"invalid"}},
                    std::ios_base::failure);
}

TEST_CASE("Reflect structs", "[reflect]") {
  wgsl_reflect::Reflect reflect{load_file("simple.wgsl")};

  REQUIRE(reflect.structures().size() == 2);

  auto vertexInput = reflect.structure("VertexInput");
  REQUIRE(vertexInput.name == "VertexInput");
  REQUIRE(vertexInput.members.size() == 2);

  auto vertexOutput = reflect.structure("VertexOutput");
  REQUIRE(vertexOutput.name == "VertexOutput");
  REQUIRE(vertexOutput.members.size() == 2);
}

TEST_CASE("Reflect functions", "[reflect]") {
  wgsl_reflect::Reflect reflect{load_file("simple.wgsl")};

  REQUIRE(reflect.functions().size() == 3);

  auto vs_main = reflect.function("vs_main");
  REQUIRE(vs_main.name == "vs_main");
  REQUIRE(vs_main.inputs.size() == 2);

  REQUIRE(vs_main.inputs[0].name == "position");
  REQUIRE(vs_main.inputs[0].type == "vec3<f32>");
  REQUIRE(vs_main.inputs[0].attributes.size() == 1);
  REQUIRE(vs_main.inputs[0].attributes[0].name == "location");
  REQUIRE(vs_main.inputs[0].attributes[0].value == "0");

  REQUIRE(vs_main.inputs[1].name == "color");
  REQUIRE(vs_main.inputs[1].type == "vec3<f32>");
  REQUIRE(vs_main.inputs[1].attributes.size() == 1);
  REQUIRE(vs_main.inputs[1].attributes[0].name == "location");
  REQUIRE(vs_main.inputs[1].attributes[0].value == "1");

  auto fs_main = reflect.function("fs_main");
  REQUIRE(fs_main.name == "fs_main");
  REQUIRE(fs_main.inputs.size() == 2);

  REQUIRE(fs_main.inputs[0].name == "clip_position");
  REQUIRE(fs_main.inputs[0].type == "vec4<f32>");
  REQUIRE(fs_main.inputs[0].attributes.size() == 1);
  REQUIRE(fs_main.inputs[0].attributes[0].name == "builtin");
  REQUIRE(fs_main.inputs[0].attributes[0].value == "position");

  REQUIRE(fs_main.inputs[1].name == "color");
  REQUIRE(fs_main.inputs[1].type == "vec3<f32>");
  REQUIRE(fs_main.inputs[1].attributes.size() == 1);
  REQUIRE(fs_main.inputs[1].attributes[0].name == "location");
  REQUIRE(fs_main.inputs[1].attributes[0].value == "0");

  auto other = reflect.function("other");
  REQUIRE(other.name == "other");
  REQUIRE(other.inputs.size() == 2);

  REQUIRE(other.inputs[0].name == "a");
  REQUIRE(other.inputs[0].type == "i32");
  REQUIRE(other.inputs[0].attributes.size() == 0);

  REQUIRE(other.inputs[1].name == "b");
  REQUIRE(other.inputs[1].type == "i32");
  REQUIRE(other.inputs[1].attributes.size() == 0);
}

TEST_CASE("Reflect entrypoints", "[reflect]") {
  wgsl_reflect::Reflect reflect{load_file("simple.wgsl")};

  REQUIRE(reflect.entries().vertex.size() == 1);
  REQUIRE(reflect.entries().fragment.size() == 1);
  REQUIRE(reflect.entries().compute.size() == 1);

  REQUIRE(reflect.vertex(0).name == "vs_main");
  REQUIRE(reflect.vertex(0).name == reflect.function("vs_main").name);
  REQUIRE(reflect.fragment(0).name == "fs_main");
  REQUIRE(reflect.fragment(0).name == reflect.function("fs_main").name);
  REQUIRE(reflect.compute(0).name == "other");
  REQUIRE(reflect.compute(0).name == reflect.function("other").name);
}
