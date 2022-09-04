#include "catch2/catch_all.hpp"

#include <tree_sitter_wgsl.h>

#include "util.hpp"

#include "cppts/node.hpp"
#include "cppts/parser.hpp"
#include "cppts/query.hpp"
#include "cppts/tree.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

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
    CHECK(countMatches(queryCursor) == 4);
  }

  SECTION("Tree query") {
    auto cursor = tree.query(query_string);
    CHECK(countMatches(cursor) == 4);
  }

  SECTION("Node query") {
    auto cursor = tree.rootNode().query(query_string);
    CHECK(countMatches(cursor) == 4);
  }

  SECTION("Alternative match getter") {
    auto cursor = tree.rootNode().query(query_string);
    CHECK(cursor.nextMatch());
    CHECK(cursor.nextMatch());
    CHECK(cursor.nextMatch());
    CHECK(cursor.nextMatch());
    CHECK_FALSE(cursor.nextMatch());
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

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "vs_main");

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "other");

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "fs_main");

    CHECK(!queryCursor.nextMatch(match));
  }

  SECTION("Extra capture") {
    std::string query_string =
        "(function_declaration (attribute (identifier) @functype) name: "
        "(identifier) @funcname) @thefunc";

    auto queryCursor = tree.query(query_string);

    cppts::Match match;
    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "vs_main");
    CHECK(match["functype"].node().str() == "vertex");

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "other");
    CHECK(match["functype"].node().str() == "compute");

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "other");
    CHECK(match["functype"].node().str() == "workgroup_size");

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "fs_main");
    CHECK(match["functype"].node().str() == "fragment");

    CHECK(!queryCursor.nextMatch(match));
  }

  SECTION("Optional capture") {
    std::string query_string =
        "(function_declaration (attribute (identifier) @functype)? name: "
        "(identifier) @funcname) @thefunc";

    auto queryCursor = tree.query(query_string);

    cppts::Match match;

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "vs_main");
    CHECK(match["functype"].node().str() == "vertex");
    CHECK(match.has("funcname"));
    CHECK(match.has("functype"));
    CHECK(match.maybe_capture("functype") != std::nullopt);

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "other");
    CHECK(match.has("funcname"));
    CHECK(match.has("functype"));
    CHECK(match.maybe_capture("functype") != std::nullopt);

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "other");
    CHECK(match.has("funcname"));
    CHECK(match.has("functype"));
    CHECK(match.maybe_capture("functype") != std::nullopt);

    CHECK(queryCursor.nextMatch(match));
    CHECK(match["funcname"].node().str() == "fs_main");
    CHECK(match["functype"].node().str() == "fragment");
    CHECK(match.has("funcname"));
    CHECK(match.has("functype"));
    CHECK(match.maybe_capture("functype") != std::nullopt);

    CHECK_FALSE(queryCursor.nextMatch(match));
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
    CHECK(a["funcname"].node().str() == "other");
    CHECK(a.has("param"));
    CHECK(a["param.name"].node().str() == "a");
    CHECK(a["param.type"].node().str() == "i32");

    auto b = cursor.nextMatch().value();
    CHECK(b["funcname"].node().str() == "other");
    CHECK(a.has("param"));
    CHECK(b.has("param"));
    CHECK(b["param.name"].node().str() == "b");
    CHECK(b["param.type"].node().str() == "f32");

    CHECK_FALSE(cursor.nextMatch());
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

    CHECK(countMatches(cursor) == 0);
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
  CHECK(match["attribute"].node().str() == "compute");

  {
    auto _cursor =
        match["attr"].node().query("(attribute (identifier) (_)? @args)");
    auto a = _cursor.nextMatch().value();
    CHECK_FALSE(a.has("args"));
  }

  match = cursor.nextMatch().value();
  CHECK(match["attribute"].node().str() == "workgroup_size");

  {
    auto _cursor =
        match["attr"].node().query("(attribute (identifier) (_)? @arg)");
    auto _match = _cursor.nextMatch().value();
    CHECK(_match.has("arg"));
    CHECK(_match["arg"].node().str() == "8");
    _match = _cursor.nextMatch().value();
    CHECK(_match.has("arg"));
    CHECK(_match["arg"].node().str() == "4");
    _match = _cursor.nextMatch().value();
    CHECK(_match.has("arg"));
    CHECK(_match["arg"].node().str() == "1");
  }
}

TEST_CASE("Node navigation", "[parsing]") {
  std::string source = R"WGSL(
    fn other() -> i32 {
        return a + b;
    })WGSL";
  cppts::Tree tree{parser, source};

  SECTION("Direct") {
    CHECK_FALSE(tree.rootNode().isNull());
    CHECK(tree.rootNode().childCount() == 1);
    auto decl = tree.rootNode().child(0);
    CHECK_FALSE(decl.isNull());
    CHECK(decl.type() == "function_declaration"s);
    CHECK(decl.childCount() == 6);
    CHECK(decl.namedChildCount() == 3);

    CHECK_THROWS_AS(tree.rootNode().child(1), std::out_of_range);

    std::vector<std::string> childNames;
    std::vector<std::string> childTypes;
    for (uint32_t i = 0; i < decl.childCount(); i++) {
      childNames.emplace_back(decl.child(i).str());
      childTypes.push_back(decl.child(i).type());
    }
    CHECK(childNames ==
          std::vector<std::string>{"fn", "other", "(", ")", "-> i32", R"S({
        return a + b;
    })S"});

    auto fname = decl.child(1);
    CHECK(fname.prevNamedSibling().isNull());
    CHECK(fname.nextNamedSibling() == decl.child(4));
    CHECK(decl.child(0).prevSibling().isNull());
    CHECK_FALSE(decl.child(0).prevSibling());
    CHECK_FALSE(decl.child(0).isNamed());
    CHECK(decl.child(1).isNamed());

    CHECK_FALSE(decl.child(decl.childCount() - 1).nextSibling());
    CHECK_FALSE(decl.child(decl.childCount() - 1).nextNamedSibling());

    CHECK(decl.child(0).str() == "fn"s);
    CHECK(decl.child(0).type() == "fn"s);
    CHECK(decl.namedChild(0) == decl.child("name"));

    CHECK(decl.child(2) == decl.child(1).nextSibling());
    CHECK(decl.child(2) == decl.child(3).prevSibling());

    CHECK(decl.child("name").str() == "other"s);
    CHECK(decl.child("type").str() == "-> i32"s);
    auto body = decl.child("body").str();
    CHECK(body.find("return a + b;") != std::string::npos);

    CHECK_THROWS_AS(decl.child("blubb"), std::out_of_range);
  }

  SECTION("Iterate children") {
    auto decl = tree.rootNode().child(0);

    uint32_t i = 0;
    for (auto child : decl.children()) {
      CHECK(child == decl.child(i));
      i++;
    }

    i = 0;
    for (auto child : decl.namedChildren()) {
      CHECK(child == decl.namedChild(i));
      i++;
    }
  }

  SECTION("Cursor") {
    auto cursor = tree.rootNode().cursor();
    CHECK(cursor.currentNode() == tree.rootNode());
    CHECK_FALSE(cursor.currentNode().isNull());

    cursor.firstChild();
    CHECK(cursor.currentNode() == tree.rootNode().child(0));
    cursor.firstChild();
    CHECK(cursor.currentNode() == tree.rootNode().child(0).child(0));
    cursor.nextSibling();
    CHECK(cursor.currentNode() == tree.rootNode().child(0).child(1));
    cursor.parent();
    CHECK(cursor.currentNode() == tree.rootNode().child(0));
    CHECK(cursor.currentFieldName() == nullptr);
    cursor.firstChild().nextSibling();
    CHECK(cursor.currentNode().str() == "other"s);
    CHECK(cursor.currentFieldName() == "name"s);
  }
}

TEST_CASE("Parsing functionality", "[parser]") {
  {
    cppts::Tree tree{parser, "var u_texture: texture_2d<f32>;"};

    auto tdecl =
        tree.rootNode().namedChild(0).namedChild(0).namedChild(0).namedChild(1);
    CHECK(tdecl.type() == "type_declaration"s);
    CHECK(tdecl.namedChildCount() == 0);
    CHECK(tdecl.str() == "texture_2d<f32>");
  }

  {
    cppts::Tree tree{parser, "var u_mat: mat4x4<f32>;"};

    auto tdecl =
        tree.rootNode().namedChild(0).namedChild(0).namedChild(0).namedChild(1);
    CHECK(tdecl.type() == "type_declaration"s);
    CHECK(tdecl.namedChildCount() == 1);
    CHECK(tdecl.namedChild(0).type() == "type_declaration"s);

    CHECK(tdecl.str() == "mat4x4<f32>");
    CHECK(tdecl.namedChild(0).str() == "f32");
  }
}