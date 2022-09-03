#include "catch2/catch_all.hpp"

#include "wgsl_reflect/reflect.hpp"

#include <nlohmann/json.hpp>
#include <tree_sitter_wgsl.h>

#include "util.hpp"

#include <iostream>

using namespace std::string_literals;

TEST_CASE("Convert to JSON", "[json]") {
  SECTION("Simple") {
    std::string source = load_file("simple.wgsl");
    wgsl_reflect::Reflect reflect{source};
    nlohmann::json j;
    j = reflect;
    std::stringstream ss;
    ss << j.dump(2) << std::endl;
  }

  SECTION("Reference") {
    std::string source = load_file("reference.wgsl");
    wgsl_reflect::Reflect reflect{source};
    nlohmann::json j;
    j = reflect;
    std::stringstream ss;
    ss << j.dump(2) << std::endl;
  }
}