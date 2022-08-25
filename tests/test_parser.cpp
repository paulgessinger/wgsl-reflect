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

using namespace std::string_literals;

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
    REQUIRE(countMatches(queryCursor) == 4);
  }

  SECTION("Tree query") {
    auto cursor = tree.query(query_string);
    REQUIRE(countMatches(cursor) == 4);
  }

  SECTION("Node query") {
    auto cursor = tree.rootNode().query(query_string);
    REQUIRE(countMatches(cursor) == 4);
  }

  SECTION("Alternative match getter") {
    auto cursor = tree.rootNode().query(query_string);
    REQUIRE(cursor.nextMatch());
    REQUIRE(cursor.nextMatch());
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
    REQUIRE(match["funcname"].node().str() == "other");
    REQUIRE(match["functype"].node().str() == "compute");

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "other");
    REQUIRE(match["functype"].node().str() == "workgroup_size");

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
    REQUIRE(match.has("functype"));
    REQUIRE(match.maybe_capture("functype") != std::nullopt);

    REQUIRE(queryCursor.nextMatch(match));
    REQUIRE(match["funcname"].node().str() == "other");
    REQUIRE(match.has("funcname"));
    REQUIRE(match.has("functype"));
    REQUIRE(match.maybe_capture("functype") != std::nullopt);

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
          name: (identifier) @funcname
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
    REQUIRE(a["funcname"].node().str() == "other");
    REQUIRE(a.has("param"));
    REQUIRE(a["param.name"].node().str() == "a");
    REQUIRE(a["param.type"].node().str() == "i32");

    auto b = cursor.nextMatch().value();
    REQUIRE(b["funcname"].node().str() == "other");
    REQUIRE(a.has("param"));
    REQUIRE(b.has("param"));
    REQUIRE(b["param.name"].node().str() == "b");
    REQUIRE(b["param.type"].node().str() == "f32");

    REQUIRE_FALSE(cursor.nextMatch());
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

TEST_CASE("Multiple attributes", "[parsing]") {
  std::string source = R"WGSL(
    @compute @workgroup_size(8,4,1)
    fn sorter() {}
    )WGSL";
  cppts::Tree tree{parser, source};

  auto cursor = tree.query(R"Q(
    (function_declaration
      (attribute (identifier) @attribute)? @attr
    )
  )Q");

  auto match = cursor.nextMatch().value();
  REQUIRE(match["attribute"].node().str() == "compute");

  {
    auto _cursor =
        match["attr"].node().query("(attribute (identifier) (_)? @args)");
    auto a = _cursor.nextMatch().value();
    REQUIRE_FALSE(a.has("args"));
  }

  match = cursor.nextMatch().value();
  REQUIRE(match["attribute"].node().str() == "workgroup_size");

  {
    auto _cursor =
        match["attr"].node().query("(attribute (identifier) (_)? @arg)");
    auto _match = _cursor.nextMatch().value();
    REQUIRE(_match.has("arg"));
    REQUIRE(_match["arg"].node().str() == "8");
    _match = _cursor.nextMatch().value();
    REQUIRE(_match.has("arg"));
    REQUIRE(_match["arg"].node().str() == "4");
    _match = _cursor.nextMatch().value();
    REQUIRE(_match.has("arg"));
    REQUIRE(_match["arg"].node().str() == "1");
  }
}

TEST_CASE("Node navigation", "[parsing]") {
  std::string source = R"WGSL(
    fn other() -> i32 {
        return a + b;
    })WGSL";
  cppts::Tree tree{parser, source};

  SECTION("Direct") {
    REQUIRE_FALSE(tree.rootNode().isNull());
    REQUIRE(tree.rootNode().childCount() == 1);
    auto decl = tree.rootNode().child(0);
    REQUIRE_FALSE(decl.isNull());
    REQUIRE(decl.type() == "function_declaration"s);
    REQUIRE(decl.childCount() == 6);
    REQUIRE(decl.namedChildCount() == 3);

    std::vector<std::string> childNames;
    std::vector<std::string> childTypes;
    for (uint32_t i = 0; i < decl.childCount(); i++) {
      childNames.emplace_back(decl.child(i).str());
      childTypes.push_back(decl.child(i).type());
    }
    REQUIRE(childNames ==
            std::vector<std::string>{"fn", "other", "(", ")", "-> i32", R"S({
        return a + b;
    })S"});

    auto fname = decl.child(1);
    REQUIRE(fname.prevNamedSibling().isNull());
    REQUIRE(fname.nextNamedSibling() == decl.child(4));
    REQUIRE(decl.child(0).prevSibling().isNull());
    REQUIRE_FALSE(decl.child(0).prevSibling());
    REQUIRE_FALSE(decl.child(0).isNamed());
    REQUIRE(decl.child(1).isNamed());

    REQUIRE(decl.child(0).str() == "fn"s);
    REQUIRE(decl.child(0).type() == "fn"s);
    REQUIRE(decl.namedChild(0) == decl.child("name"));

    REQUIRE(decl.child(2) == decl.child(1).nextSibling());
    REQUIRE(decl.child(2) == decl.child(3).prevSibling());

    REQUIRE(decl.child("name").str() == "other"s);
    REQUIRE(decl.child("type").str() == "-> i32"s);
    auto body = decl.child("body").str();
    REQUIRE(body.find("return a + b;") != std::string::npos);
  }

  SECTION("Iterate children") {
    auto decl = tree.rootNode().child(0);

    uint32_t i = 0;
    for (auto child : decl.children()) {
      REQUIRE(child == decl.child(i));
      i++;
    }


    i = 0;
    for (auto child : decl.namedChildren()) {
      REQUIRE(child == decl.namedChild(i));
      i++;
    }
  }

  SECTION("Cursor") {
    auto cursor = tree.rootNode().cursor();
    REQUIRE(cursor.currentNode() == tree.rootNode());
    REQUIRE_FALSE(cursor.currentNode().isNull());

    cursor.firstChild();
    REQUIRE(cursor.currentNode() == tree.rootNode().child(0));
    cursor.firstChild();
    REQUIRE(cursor.currentNode() == tree.rootNode().child(0).child(0));
    cursor.nextSibling();
    REQUIRE(cursor.currentNode() == tree.rootNode().child(0).child(1));
    cursor.parent();
    REQUIRE(cursor.currentNode() == tree.rootNode().child(0));
    REQUIRE(cursor.currentFieldName() == nullptr);
    cursor.firstChild().nextSibling();
    REQUIRE(cursor.currentNode().str() == "other"s);
    REQUIRE(cursor.currentFieldName() == "name"s);
  }
}