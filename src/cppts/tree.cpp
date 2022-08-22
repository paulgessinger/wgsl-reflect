#include "cppts/tree.hpp"

#include "cppts/node.hpp"

namespace cppts {
Node Tree::rootNode() { return Node{*this, ts_tree_root_node(m_tree)}; }
}  // namespace cppts