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

class Cursor {
 public:
  explicit Cursor(Node node) : m_tree{node.getTree()} {
    m_cursor = ts_tree_cursor_new(node.getNode());
  }

  Cursor& firstChild() {
    bool r = ts_tree_cursor_goto_first_child(&m_cursor);
    if (!r) {
      throw std::runtime_error{"No first child"};
    }
    return *this;
  }

  Cursor& nextSibling() {
    bool r = ts_tree_cursor_goto_next_sibling(&m_cursor);
    if (!r) {
      throw std::runtime_error{"No next sibling"};
    }
    return *this;
  }

  Cursor& parent() {
    bool r = ts_tree_cursor_goto_parent(&m_cursor);
    if (!r) {
      throw std::runtime_error{"No parent"};
    }
    return *this;
  }

  Node currentNode() {
    return Node{m_tree, ts_tree_cursor_current_node(&m_cursor)};
  }
  const char* currentFieldName() {
    return ts_tree_cursor_current_field_name(&m_cursor);
  }
  TSFieldId currentFieldId() {
    return ts_tree_cursor_current_field_id(&m_cursor);
  }

  ~Cursor() { ts_tree_cursor_delete(&m_cursor); }

 private:
  TSTreeCursor m_cursor;
  Tree& m_tree;
};

}  // namespace cppts