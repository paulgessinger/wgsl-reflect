#include "cppts/node.hpp"

#include "cppts/tree.hpp"

namespace cppts {
std::string_view Node::code() const {
  return std::string_view{m_tree.source().data() + start(), length()};
}
std::string Node::ast() const { return std::string{ts_node_string(m_node)}; }
}  // namespace cppts