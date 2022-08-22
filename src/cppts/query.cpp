#include "cppts/query.hpp"

#include "cppts/node.hpp"

namespace cppts {
QueryCursor Query::exec(Node node) { return QueryCursor(*this, node); }
}  // namespace cppts