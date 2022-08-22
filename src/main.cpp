
#include "cppts/node.hpp"
#include "cppts/parser.hpp"
#include "cppts/query.hpp"
#include "cppts/tree.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include <tree_sitter/api.h>

extern "C" {
extern const TSLanguage *tree_sitter_wgsl(void);
}

int main() {
  std::cout << "HALLO" << std::endl;

  cppts::Parser parser{tree_sitter_wgsl()};

  std::ifstream ifs{"test.wgsl"};
  if (!ifs.good()) {
    std::cerr << "couldn't read" << std::endl;
    return 1;
  }
  std::stringstream sstream;
  sstream << ifs.rdbuf();

  std::string source = sstream.str();

  //  std::cout << source << std::endl;

  cppts::Tree tree{
      parser,
      source,
  };

  //  std::cout << tree.rootNode().ast() << "\n---\n" << std::endl;

  std::string query_string =
      "(function_declaration (attribute (identifier) @functype) name: "
      "(identifier) @funcname) @thefunc";

  cppts::Query query{tree_sitter_wgsl(), query_string};

  cppts::QueryCursor queryCursor = query.exec(tree.rootNode());

  cppts::Match match;
  while (queryCursor.nextMatch(match)) {
    std::cout << "MATCH" << std::endl;

    std::cout << "- " << match["funcname"].node().code() << " ("
              << match["functype"].node().code() << ")" << std::endl;
    //    std::cout << match.capture("functype").node().code() << std::endl;
    std::cout << match["thefunc"].node().code() << std::endl;
    std::cout << match["thefunc"].node().ast() << std::endl;

    //    cppts::Capture capture = match.capture("funcname");
    //    std::cout << " - funcname: " << capture.node().code() << std::endl;
    //    std::cout << " - thefunc: " << match.capture("thefunc").node().code()
    //              << std::endl;
  }

  //  TSQueryMatch match;
  //
  //  auto nodeString = [](TSNode node) -> std::string {
  //    char *s = ts_node_string(node);
  //    std::string str{s};
  //    free(s);
  //    return str;
  //  };
  //
  //  auto nodeValue = [&source](TSNode node) -> std::string_view {
  //    uint32_t start = ts_node_start_byte(node);
  //    uint32_t end = ts_node_end_byte(node);
  //    uint32_t len = end - start;
  //    return std::string_view{source.data() + start, end - start};
  //  };
  //
  //  while (ts_query_cursor_next_match(cursor, &match)) {
  //    std::cout << "MATCH!" << std::endl;
  //
  //    std::cout << "capcount: " << match.capture_count << std::endl;
  //    for (uint32_t i = 0; i < match.capture_count; i++) {
  //      const TSQueryCapture &capture = match.captures[i];
  //      std::cout << " - " << capture.index << std::endl;
  //      //            capture.node
  //      std::cout << nodeString(capture.node) << std::endl;
  //      std::cout << nodeValue(capture.node) << std::endl;
  //    }
  //
  //    //        while(ts_query_cursor_next_capture(cursor, ))
  //  }
  //
  std::cout << "DONE" << std::endl;
}