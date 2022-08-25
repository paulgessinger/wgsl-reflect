#define CATCH_CONFIG_MAIN

#include "catch2/catch_all.hpp"

#include <tree_sitter_wgsl.h>

#include "util.hpp"

#include "cppts/node.hpp"
#include "cppts/parser.hpp"
#include "cppts/query.hpp"
#include "cppts/tree.hpp"

#include <filesystem>
#include <fstream>

cppts::Parser parser{tree_sitter_wgsl()};

size_t countMatches(cppts::QueryCursor& cursor) {
  size_t n = 0;
  cppts::Match match;
  while (cursor.nextMatch(match)) {
    n++;
  }
  return n;
}

TEST_CASE("Query API", "[parsing]") {
  std::string source = load_file("simple.wgsl");

  cppts::Tree tree{parser, source};

  std::string query_string =
      "(function_declaration (attribute (identifier) @functype) name: "
      "(identifier) @funcname) @thefunc";

  SECTION("Verbose API") {
    auto query = cppts::Query::create(tree_sitter_wgsl(), query_string);
    cppts::QueryCursor queryCursor = query->exec(tree.rootNode());
    REQUIRE(countMatches(queryCursor) == 2);
  }

  SECTION("Tree query") {
    auto cursor = tree.query(query_string);
    REQUIRE(countMatches(cursor) == 2);
  }

  SECTION("Node query") {
    auto cursor = tree.rootNode().query(query_string);
    REQUIRE(countMatches(cursor) == 2);
  }

  SECTION("Alternative match getter") {
    auto cursor = tree.rootNode().query(query_string);
    REQUIRE(cursor.nextMatch());
    REQUIRE(cursor.nextMatch());
    REQUIRE_FALSE(cursor.nextMatch());
  }
}

TEST_CASE("Query captures", "[parsing]") {
  cppts::Tree tree{parser, load_file("simple.wgsl")};

  SECTION("All functions") {
    std::string query_string =
        "(function_declaration name: "
        "(identifier) @funcname) @thefunc";

    auto queryCursor = tree.query(query_string);

    cppts::Match match;

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "vs_main");

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "other");

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "fs_main");

    REQUIRE(!queryCursor.nextMatch(match));
  }

  SECTION("Extra capture") {
    std::string query_string =
        "(function_declaration (attribute (identifier) @functype) name: "
        "(identifier) @funcname) @thefunc";

    auto queryCursor = tree.query(query_string);

    cppts::Match match;
    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "vs_main");
    REQUIRE(match["functype"].node().str() == "vertex");

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "fs_main");
    REQUIRE(match["functype"].node().str() == "fragment");

    REQUIRE(!queryCursor.nextMatch(match));
  }

  SECTION("Optional capture") {
    std::string query_string =
        "(function_declaration (attribute (identifier) @functype)? name: "
        "(identifier) @funcname) @thefunc";

    auto queryCursor = tree.query(query_string);

    cppts::Match match;

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "vs_main");
    REQUIRE(match["functype"].node().str() == "vertex");
    REQUIRE(match.has("funcname"));
    REQUIRE(match.has("functype"));
    REQUIRE(match.maybe_capture("functype") != std::nullopt);

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "other");
    REQUIRE(match.has("funcname"));
    REQUIRE_FALSE(match.has("functype"));
    REQUIRE(match.maybe_capture("functype") == std::nullopt);

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "fs_main");
    REQUIRE(match["functype"].node().str() == "fragment");
    REQUIRE(match.has("funcname"));
    REQUIRE(match.has("functype"));
    REQUIRE(match.maybe_capture("functype") != std::nullopt);

    REQUIRE_FALSE(queryCursor.nextMatch(match));
  }

  SECTION("Repeating capture") {
    std::string source = R"WGSL(
  fn other(a: i32, b: f32) -> i32 {
      return a + b;
  })WGSL";
    cppts::Tree _tree{parser, source};

    auto cursor = _tree.query(R"Q(
        (function_declaration
          (parameter_list
            (parameter
              (variable_identifier_declaration
                name: (identifier) @param.name
                type: (type_declaration) @param.type
              )
            )? @param
          )
        ))Q");

    auto a = cursor.nextMatch().value();
    REQUIRE(a.has("param"));
    REQUIRE(a["param.name"].node().str() == "a");
    REQUIRE(a["param.type"].node().str() == "i32");

    auto b = cursor.nextMatch().value();
    REQUIRE(b.has("param"));
    REQUIRE(b["param.name"].node().str() == "b");
    REQUIRE(b["param.type"].node().str() == "f32");
  }

  SECTION("Repeating capture, no match") {
    std::string source = R"WGSL(
    fn other() -> i32 {
        return a + b;
    })WGSL";
    cppts::Tree _tree{parser, source};

    auto cursor = _tree.query(R"Q(
          (function_declaration
            (parameter_list
              (parameter
                (variable_identifier_declaration
                  name: (identifier) @param.name
                  type: (type_declaration (identifier) @param.type)
                )
              )? @param
            )
          ))Q");

    REQUIRE(countMatches(cursor) == 0);
  }
}
