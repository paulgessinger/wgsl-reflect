#include "cppts/query.hpp"

#include "cppts/node.hpp"

namespace cppts {
QueryCursor Query::exec(Node node) {
  return QueryCursor(shared_from_this(), node);
}
}  // namespace cppts