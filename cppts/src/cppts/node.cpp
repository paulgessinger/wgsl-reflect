#include "cppts/node.hpp"

#include "cppts/tree.hpp"

namespace cppts {
std::string_view Node::str() const {
  return std::string_view{m_tree.source().data() + start(), length()};
}
std::string Node::ast() const { return std::string{ts_node_string(m_node)}; }

QueryCursor Node::query(const std::string& query_string) {
  auto _query = Query::create(m_tree.getParser().language(), query_string);
  return _query->exec(*this);
}
Cursor Node::cursor() { return Cursor{*this}; }

bool Node::operator==(const Node& other) const {
  return ts_node_eq(m_node, other.m_node);
}

}  // namespace cppts