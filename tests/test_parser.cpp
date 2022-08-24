#define CATCH_CONFIG_MAIN

#include "catch2/catch_all.hpp"

#include <tree_sitter_wgsl.h>

#include "cppts/node.hpp"
#include "cppts/parser.hpp"
#include "cppts/query.hpp"
#include "cppts/tree.hpp"

#include <filesystem>
#include <fstream>

std::string load_file(const std::filesystem::path& fn) {
  static const std::filesystem::path test_dir =
      std::filesystem::path{__FILE__}.parent_path();
  std::stringstream ss;
  std::ifstream ifs{test_dir / fn};
  ifs.exceptions(std::ifstream::failbit);
  ss << ifs.rdbuf();
  return ss.str();
}

TEST_CASE("Simple parsing works", "[parsing]") {
  cppts::Parser parser{tree_sitter_wgsl()};

  std::string source = load_file("simple.wgsl");

  cppts::Tree tree{
      parser,
      source,
  };

  std::string query_string =
      "(function_declaration (attribute (identifier) @functype) name: "
      "(identifier) @funcname) @thefunc";

  cppts::Query query{tree_sitter_wgsl(), query_string};

  cppts::QueryCursor queryCursor = query.exec(tree.rootNode());

  cppts::Match match;
  REQUIRE(queryCursor.nextMatch(match));
  REQUIRE(match["funcname"].node().str() == "vs_main");
  REQUIRE(match["functype"].node().str() == "vertex");

  REQUIRE(queryCursor.nextMatch(match));
  REQUIRE(match["funcname"].node().str() == "fs_main");
  REQUIRE(match["functype"].node().str() == "fragment");

  REQUIRE(!queryCursor.nextMatch(match));
}