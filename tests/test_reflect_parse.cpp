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

TEST_CASE("Parse function", "[reflect]") {
  cppts::Parser parser{tree_sitter_wgsl()};

  SECTION("Invalid input") {
    cppts::Tree tree{parser, "struct A { a: i32 };"};
    CHECK_THROWS(wgsl_reflect::Function{tree.rootNode()});
    CHECK_THROWS(wgsl_reflect::Function{tree.rootNode().child(0)});
  }

  SECTION("Multiple inputs") {
    std::string source = R"WGSL(
  fn other(a: i32, b:i32) -> i32 {
      return a + b;
  })WGSL";
    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode().child(0)};

    CHECK(function.name == "other");
    CHECK(function.inputs.size() == 2);
    CHECK(function.inputs[0].name == "a");
    CHECK(function.inputs[0].type == "i32");
    CHECK(function.inputs[1].name == "b");
    CHECK(function.inputs[1].type == "i32");
  }

  SECTION("No inputs") {
    std::string source = R"WGSL(
  fn noarg() -> i32 {
      return a + b;
  })WGSL";
    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode().child(0)};

    CHECK(function.name == "noarg");
    CHECK(function.inputs.size() == 0);
  }

  SECTION("Attributes") {
    std::string source = R"WGSL(
      @compute @workgroup_size(8,4,1)
      fn other(a: i32, b:i32) -> int32 {
          return a + b;
      }
    )WGSL";

    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode().child(0)};
    CHECK(function.name == "other");
    CHECK(function.inputs.size() == 2);
    CHECK(function.attribute("compute").value() == ""s);
    CHECK_FALSE(function.attribute("vertex").has_value());
    CHECK(function.attribute("workgroup_size").value() == "8,4,1"s);
  }

  SECTION("Flat inputs with locations") {
    std::string source = R"WGSL(
    @fragment
    fn main(@location(0) in1: i32, @location(2) @interpolate(flat) in2: f32) -> i32 {
        return a + b;
    })WGSL";

    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode().child(0)};
    CHECK(function.name == "main");
    CHECK(function.inputs.size() == 2);
    CHECK(function.attribute("fragment").value() == ""s);
    CHECK_FALSE(function.attribute("vertex").has_value());

    auto& in1 = function.inputs[0];
    CHECK(in1.name == "in1");
    CHECK(in1.type == "i32");
    CHECK(in1.attributes.size() == 1);
    CHECK(in1.attributes[0].name == "location");
    CHECK(in1.attributes[0].value == "0");

    auto& in2 = function.inputs[1];
    CHECK(in2.name == "in2");
    CHECK(in2.type == "f32");
    CHECK(in2.attributes.size() == 2);
    CHECK(in2.attributes[0].name == "location");
    CHECK(in2.attributes[0].value == "2");
    CHECK(in2.attributes[1].name == "interpolate");
    CHECK(in2.attributes[1].value == "flat");
  }

  SECTION("Structure parameter not resolved") {
    std::string source = R"WGSL(
    @vertex
    fn main(input: VertexInput) -> VertexOutput {
        return a + b;
    })WGSL";

    cppts::Tree tree{parser, source};
    wgsl_reflect::Function function{tree.rootNode().child(0)};
    CHECK(function.name == "main");
    CHECK(function.inputs.size() == 1);
    CHECK(function.inputs[0].name == "input");
    CHECK(function.inputs[0].type == "VertexInput");
  }

  SECTION("Parse single struct input") {
    std::string source = R"WGSL(
      struct VertexInput {
        @location(0) position: vec3<f32>,
        @location(1) color: vec3<f32>,
      };

      @vertex
      fn main(input: VertexInput) -> VertexOutput {
          return a + b;
      }
    )WGSL";
    cppts::Tree tree{parser, source};

    wgsl_reflect::Structure _struct{tree.rootNode().namedChild(0)};

    wgsl_reflect::Function function{
        tree.rootNode().namedChild(1),
        [&](const std::string& s) -> std::optional<wgsl_reflect::Structure> {
          if (s == "VertexInput") {
            return _struct;
          } else {
            return std::nullopt;
          }
        }};

    CHECK(function.name == "main");
    CHECK(function.inputs.size() == 2);
    CHECK(function.inputs[0].name == "position");
    CHECK(function.inputs[0].type == "vec3<f32>");
    CHECK(function.inputs[0].attributes.size() == 1);
    CHECK(function.inputs[0].attributes[0].name == "location");
    CHECK(function.inputs[0].attributes[0].value == "0");
    CHECK(function.inputs[1].name == "color");
    CHECK(function.inputs[1].type == "vec3<f32>");
    CHECK(function.inputs[1].attributes.size() == 1);
    CHECK(function.inputs[1].attributes[0].name == "location");
    CHECK(function.inputs[1].attributes[0].value == "1");
  }

  SECTION("Parse multiple struct input") {
    std::string source = R"WGSL(
      struct VertexInput1 {
        @location(0) position: vec3<f32>,
      };

      struct VertexInput2 {
        @location(1) color: vec3<f32>,
      };

      @vertex
      fn main(a: VertexInput1, b: VertexInput2) -> VertexOutput {
          return a + b;
      }
    )WGSL";
    cppts::Tree tree{parser, source};

    wgsl_reflect::Structure input1{tree.rootNode().namedChild(0)};
    CHECK(input1.name == "VertexInput1");
    wgsl_reflect::Structure input2{tree.rootNode().namedChild(1)};
    CHECK(input2.name == "VertexInput2");

    wgsl_reflect::Function function{
        tree.rootNode().namedChild(2),
        [&](const std::string& s) -> std::optional<wgsl_reflect::Structure> {
          if (s == "VertexInput1") {
            return input1;
          } else {
            return input2;
          }
          return std::nullopt;
        }};

    CHECK(function.name == "main");
    CHECK(function.inputs.size() == 2);
    CHECK(function.inputs[0].name == "position");
    CHECK(function.inputs[0].type == "vec3<f32>");
    CHECK(function.inputs[0].attributes.size() == 1);
    CHECK(function.inputs[0].attributes[0].name == "location");
    CHECK(function.inputs[0].attributes[0].value == "0");
    CHECK(function.inputs[1].name == "color");
    CHECK(function.inputs[1].type == "vec3<f32>");
    CHECK(function.inputs[1].attributes.size() == 1);
    CHECK(function.inputs[1].attributes[0].name == "location");
    CHECK(function.inputs[1].attributes[0].value == "1");
  }

  SECTION("Parse mixed struct input") {
    std::string source = R"WGSL(
      struct VertexInput {
        @location(0) position: vec3<f32>,
      };

      @vertex
      fn main(@location(1) color: vec3<f32>, b: VertexInput) -> VertexOutput {
          return a + b;
      }
    )WGSL";
    cppts::Tree tree{parser, source};

    wgsl_reflect::Structure input{tree.rootNode().namedChild(0)};

    wgsl_reflect::Function function{
        tree.rootNode().namedChild(1),
        [&](const std::string& s) -> std::optional<wgsl_reflect::Structure> {
          if (s == "VertexInput") {
            return input;
          }
          return std::nullopt;
        }};

    CHECK(function.name == "main");
    CHECK(function.inputs.size() == 2);
    CHECK(function.inputs[0].name == "color");
    CHECK(function.inputs[0].type == "vec3<f32>");
    CHECK(function.inputs[0].attributes.size() == 1);
    CHECK(function.inputs[0].attributes[0].name == "location");
    CHECK(function.inputs[0].attributes[0].value == "1");
    CHECK(function.inputs[1].name == "position");
    CHECK(function.inputs[1].type == "vec3<f32>");
    CHECK(function.inputs[1].attributes.size() == 1);
    CHECK(function.inputs[1].attributes[0].name == "location");
    CHECK(function.inputs[1].attributes[0].value == "0");
  }
}

TEST_CASE("Parse struct", "[reflect]") {
  cppts::Parser parser{tree_sitter_wgsl()};

  SECTION("Invalid input") {
    cppts::Tree tree{parser, "fn add(a: i32, b: i32) -> i32 {return a+b;}"};
    CHECK_THROWS(wgsl_reflect::Structure{tree.rootNode()});
    CHECK_THROWS(wgsl_reflect::Structure{tree.rootNode().child(0)});
  }

  SECTION("Single attributes") {
    std::string source = R"WGSL(
      struct VertexInput {
        @location(0) position: vec3<f32>,
        @location(1) color: vec3<f32>,
      };
    )WGSL";
    cppts::Tree tree{parser, source};
    wgsl_reflect::Structure _struct{tree.rootNode().child(0)};

    CHECK(_struct.name == "VertexInput");
    CHECK(_struct.members.size() == 2);
    CHECK(_struct.members[0].name == "position");
    CHECK(_struct.members[0].type == "vec3<f32>");
    CHECK(_struct.members[0].attributes.size() == 1);
    CHECK(_struct.members[0].attributes[0].name == "location");
    CHECK(_struct.members[0].attributes[0].value == "0");

    CHECK(_struct.members[1].name == "color");
    CHECK(_struct.members[1].type == "vec3<f32>");
    CHECK(_struct.members[1].attributes.size() == 1);
    CHECK(_struct.members[1].attributes[0].name == "location");
    CHECK(_struct.members[1].attributes[0].value == "1");
  }

  SECTION("Multiple attributes") {
    std::string source = R"WGSL(
      struct SuperInput {
        @builtin(position) @interpolate(flat) clip_position: vec4<f32>,
        @location(0) color: vec3<f32>,
      };
    )WGSL";

    cppts::Tree tree{parser, source};
    wgsl_reflect::Structure _struct{tree.rootNode().child(0)};

    CHECK(_struct.name == "SuperInput");
    CHECK(_struct.members.size() == 2);
    CHECK(_struct.members[0].name == "clip_position");
    CHECK(_struct.members[0].type == "vec4<f32>");
    CHECK(_struct.members[0].attributes.size() == 2);
    CHECK(_struct.members[0].attributes[0].name == "builtin");
    CHECK(_struct.members[0].attributes[0].value == "position");
    CHECK(_struct.members[0].attributes[1].name == "interpolate");
    CHECK(_struct.members[0].attributes[1].value == "flat");

    CHECK(_struct.members[1].name == "color");
    CHECK(_struct.members[1].type == "vec3<f32>");
    CHECK(_struct.members[1].attributes.size() == 1);
    CHECK(_struct.members[1].attributes[0].name == "location");
    CHECK(_struct.members[1].attributes[0].value == "0");
  }
}

TEST_CASE("Parse bind groups", "[reflect]") {
  cppts::Parser parser{tree_sitter_wgsl()};

  SECTION("Uniform") {
    cppts::Tree tree{
        parser,
        "@binding(0) @group(0) var<uniform> viewUniforms: ViewUniforms;"};

    wgsl_reflect::Binding binding{tree.rootNode().child(0)};
    CHECK(binding.binding == 0);
    CHECK(binding.group == 0);
    CHECK(binding.name == "viewUniforms");
    CHECK(binding.bindingType == "buffer");
    CHECK(binding.type == "ViewUniforms");
  }

  SECTION("Storage") {
    cppts::Tree tree{
        parser,
        "@group(2) @binding(3) var<storage,read_write> storage_buffer: B;"};

    wgsl_reflect::Binding binding{tree.rootNode().child(0)};
    CHECK(binding.binding == 3);
    CHECK(binding.group == 2);
    CHECK(binding.name == "storage_buffer");
    CHECK(binding.bindingType == "buffer");
    CHECK(binding.type == "B");
  }

  SECTION("Sampler") {
    cppts::Tree tree{parser, "@binding(2) @group(0) var u_sampler: sampler;"};

    wgsl_reflect::Binding binding{tree.rootNode().child(0)};
    CHECK(binding.binding == 2);
    CHECK(binding.group == 0);
    CHECK(binding.name == "u_sampler");
    CHECK(binding.bindingType == "sampler");
    CHECK(binding.type == "sampler");
  }

  SECTION("Texture no spaces") {
    cppts::Tree tree{parser,
                     "@binding(3) @group(1) var u_texture: texture_2d<f32>;"};

    wgsl_reflect::Binding binding{tree.rootNode().child(0)};
    CHECK(binding.binding == 3);
    CHECK(binding.group == 1);
    CHECK(binding.name == "u_texture");
    CHECK(binding.bindingType == "texture_2d");
    CHECK(binding.type == "texture_2d");
  }

  SECTION("Texture with spaces") {
    cppts::Tree tree{parser,
                     "@binding(3) @group(1) var u_texture: texture_2d <f32> ;"};

    wgsl_reflect::Binding binding{tree.rootNode().child(0)};
    CHECK(binding.binding == 3);
    CHECK(binding.group == 1);
    CHECK(binding.name == "u_texture");
    CHECK(binding.bindingType == "texture_2d");
    CHECK(binding.type == "texture_2d");
  }
}