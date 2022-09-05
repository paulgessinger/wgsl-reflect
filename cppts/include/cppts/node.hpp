#pragma once

#include <range/v3/view.hpp>
#include <tree_sitter/api.h>

#include <optional>
#include <ostream>
#include <string_view>

namespace cppts {

class Tree;
class QueryCursor;
class Cursor;

class Node {
 public:
  Node(Tree& tree, TSNode node) : m_tree{&tree}, m_node{node} {}

  Node(const Node& other) = default;
  Node& operator=(const Node& other) = default;

  unsigned int start() const { return ts_node_start_byte(m_node); }

  unsigned int end() const { return ts_node_end_byte(m_node); }

  unsigned int length() const { return end() - start(); }

  std::string_view str() const;
  std::string ast(size_t indent = 2) const;

  TSNode getNode() { return m_node; }

  Tree& getTree() { return *m_tree; }

  QueryCursor query(const std::string& query_string);

  Cursor cursor();

  bool operator==(const Node& other) const;

  const char* type() const { return ts_node_type(m_node); }

  bool isNull() const { return ts_node_is_null(m_node); }

  operator bool() const { return !isNull(); }

  bool isNamed() const { return ts_node_is_named(m_node); }

  Node parent() { return Node{*m_tree, ts_node_parent(m_node)}; }

  Node child(uint32_t i) {
    if (i >= childCount()) {
      throw std::out_of_range{"Out of range child index"};
    }
    return Node{*m_tree, ts_node_child(m_node, i)};
  }
  Node child(const std::string& fieldName) {
    auto node = Node{*m_tree, ts_node_child_by_field_name(
                                  m_node, fieldName.c_str(),
                                  static_cast<uint32_t>(fieldName.size()))};
    if (node.isNull()) {
      throw std::out_of_range{"No node with name: " + fieldName};
    }
    return node;
  }

  Node namedChild(uint32_t i) {
    if (i >= namedChildCount()) {
      throw std::out_of_range{"Out of range named child index"};
    }
    return Node{*m_tree, ts_node_named_child(m_node, i)};
  }

  uint32_t childCount() const { return ts_node_child_count(m_node); }

  uint32_t namedChildCount() const { return ts_node_named_child_count(m_node); }

  Node nextSibling() { return Node{*m_tree, ts_node_next_sibling(m_node)}; }

  Node prevSibling() { return Node{*m_tree, ts_node_prev_sibling(m_node)}; }

  auto children() {
    return ranges::iota_view{0u, childCount()} |
           ranges::views::transform([&](uint32_t i) { return child(i); });
  }

  auto namedChildren() {
    return ranges::iota_view{0u, namedChildCount()} |
           ranges::views::transform([&](uint32_t i) { return namedChild(i); });
  }

  std::optional<Node> firstChildOfType(const std::string& type) {
    for (auto child : children()) {
      if (child.type() == type) {
        return child;
      }
    }
    return std::nullopt;
  }

  Node nextNamedSibling() {
    return Node{*m_tree, ts_node_next_named_sibling(m_node)};
  }

  Node prevNamedSibling() {
    return Node{*m_tree, ts_node_prev_named_sibling(m_node)};
  }

 private:
  Tree* m_tree;
  TSNode m_node;
};

inline std::ostream& operator<<(std::ostream& os, const Node& node) {
  os << node.str();

  return os;
}

}  // namespace cppts