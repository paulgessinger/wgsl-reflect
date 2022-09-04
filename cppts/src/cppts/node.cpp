#include "cppts/node.hpp"

#include "cppts/tree.hpp"

#include <functional>
#include <sstream>

namespace cppts {
std::string_view Node::str() const {
  return std::string_view{m_tree->source().data() + start(), length()};
}
std::string Node::ast(size_t indent) const {
  if (indent == 0) {
    return std::string{ts_node_string(m_node)};
  }
  std::stringstream ss;

  std::function<void(Node, size_t)> print = [&](Node node, size_t depth) {
    for (size_t i = 0; i < depth * indent; i++) {
      ss << " ";
    }
    ss << "(" << node.type();
    if (node.namedChildCount() > 0) {
      ss << std::endl;
    }
    for (uint32_t c = 0; c < node.namedChildCount(); c++) {
      print(node.namedChild(c), depth + 1);
      if (c < node.namedChildCount() - 1) {
        ss << std::endl;
      }
    }
    ss << ")";
  };

  print(*this, 0);

  return ss.str();
}

QueryCursor Node::query(const std::string& query_string) {
  auto _query = Query::create(m_tree->getParser().language(), query_string);
  return _query->exec(*this);
}
Cursor Node::cursor() { return Cursor{*this}; }

bool Node::operator==(const Node& other) const {
  return ts_node_eq(m_node, other.m_node);
}

}  // namespace cppts