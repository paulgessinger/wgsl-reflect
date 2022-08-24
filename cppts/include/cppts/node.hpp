#pragma once

#include <tree_sitter/api.h>
#include <ostream>
#include <string_view>

namespace cppts {

class Tree;
class QueryCursor;

class Node {
 public:
  Node(Tree& tree, TSNode node) : m_tree{tree}, m_node{node} {}

  unsigned int start() const { return ts_node_start_byte(m_node); }

  unsigned int end() const { return ts_node_end_byte(m_node); }

  unsigned int length() const { return end() - start(); }

  std::string_view str() const;
  std::string ast() const;

  TSNode getNode() { return m_node; }

  Tree& getTree() { return m_tree; }

  QueryCursor query(const std::string& query_string);

 private:
  Tree& m_tree;
  TSNode m_node;
};

inline std::ostream& operator<<(std::ostream& os, const Node& node) {
  os << node.str();

  return os;
}

}  // namespace cppts