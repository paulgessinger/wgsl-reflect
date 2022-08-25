#pragma once

#include <tree_sitter/api.h>
#include <ostream>
#include <string_view>

namespace cppts {

class Tree;
class QueryCursor;
class Cursor;

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

  Cursor cursor();

  bool operator==(const Node& other) const;

  const char* type() const { return ts_node_type(m_node); }

  bool isNull() const { return ts_node_is_null(m_node); }

  operator bool() const { return !isNull(); }

  bool isNamed() const { return ts_node_is_named(m_node); }

  Node parent() { return Node{m_tree, ts_node_parent(m_node)}; }

  Node child(uint32_t i) { return Node{m_tree, ts_node_child(m_node, i)}; }
  Node child(const std::string& fieldName) {
    return Node{m_tree, ts_node_child_by_field_name(m_node, fieldName.c_str(),
                                                    fieldName.size())};
  }

  Node namedChild(uint32_t i) {
    return Node{m_tree, ts_node_named_child(m_node, i)};
  }

  uint32_t childCount() const { return ts_node_child_count(m_node); }

  uint32_t namedChildCount() const { return ts_node_named_child_count(m_node); }

  Node nextSibling() { return Node{m_tree, ts_node_next_sibling(m_node)}; }

  Node prevSibling() { return Node{m_tree, ts_node_prev_sibling(m_node)}; }

  Node nextNamedSibling() {
    return Node{m_tree, ts_node_next_named_sibling(m_node)};
  }

  Node prevNamedSibling() {
    return Node{m_tree, ts_node_prev_named_sibling(m_node)};
  }

  template <bool named>
  class ChildIterator {
   public:
    ChildIterator(Node& node, uint32_t index) : m_node{node}, m_index{index} {}

    bool operator==(const ChildIterator& other) const {
      return other.m_node == m_node && other.m_index == m_index;
    }

    ChildIterator& operator++() {
      m_index++;
      return *this;
    }

    Node operator*() {
      if constexpr (named) {
        return m_node.namedChild(m_index);
      } else {
        return m_node.child(m_index);
      }
    }

   private:
    Node& m_node;
    uint32_t m_index{0};
  };

  template <typename It>
  struct IteratorPair {
    It _begin;
    It _end;

    It begin() { return _begin; }
    It end() { return _end; }
  };

  IteratorPair<ChildIterator<false>> children() {
    return {ChildIterator<false>{*this, 0},
            ChildIterator<false>{*this, childCount()}};
  }

  IteratorPair<ChildIterator<true>> namedChildren() {
    return {ChildIterator<true>{*this, 0},
            ChildIterator<true>{*this, namedChildCount()}};
  }

 private:
  Tree& m_tree;
  TSNode m_node;
};

inline std::ostream& operator<<(std::ostream& os, const Node& node) {
  os << node.str();

  return os;
}

}  // namespace cppts