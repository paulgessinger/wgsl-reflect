#pragma once

#include "cppts/parser.hpp"

#include "cppts/query.hpp"

#include <tree_sitter/api.h>
#include <string_view>

namespace cppts {

class Node;

class Tree {
 public:
  Tree(Parser& parser, std::string_view source)
      : m_source{source}, m_parser{parser} {
    m_tree = ts_parser_parse_string(parser.parser(), nullptr, m_source.data(),
                                    static_cast<uint32_t>(m_source.size()));
  }

  ~Tree() { ts_tree_delete(m_tree); }

  Node rootNode();

  std::string_view source() const { return m_source; }

  Parser& getParser() { return m_parser; }

  QueryCursor query(const std::string& query_string) {
    return rootNode().query(query_string);
  }

 private:
  std::string_view m_source;
  Parser& m_parser;
  TSTree* m_tree{nullptr};
};

}  // namespace cppts