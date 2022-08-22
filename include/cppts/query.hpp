#pragma once

#include "cppts/node.hpp"

#include <tree_sitter/api.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace cppts {

class QueryCursor;
class Node;

class Query {
 public:
  Query(const TSLanguage* language, std::string_view query) {
    uint32_t error_offset;
    TSQueryError error_type;
    m_query = ts_query_new(language, query.data(),
                           static_cast<uint32_t>(query.size()), &error_offset,
                           &error_type);
    if (error_type != TSQueryErrorNone) {
      throw std::invalid_argument{"Query invalid"};
    }
  }

  ~Query() { ts_query_delete(m_query); }

  TSQuery* query() { return m_query; }

  QueryCursor exec(Node node);

 private:
  TSQuery* m_query{nullptr};
};

class Match;
class Capture {
 public:
  Capture(Match& match, const TSQueryCapture& capture);

  Node node();

 private:
  Match& m_match;
  const TSQueryCapture& m_capture;
};

class Match {
 public:
  Match() = default;
  Match(QueryCursor& queryCursor, TSQueryMatch match)
      : m_queryCursor{&queryCursor}, m_match{match} {}

  Capture capture(const std::string& name);

  Capture operator[](const std::string& name);

  QueryCursor& queryCursor() { return *m_queryCursor; }

 private:
  QueryCursor* m_queryCursor{nullptr};
  TSQueryMatch m_match;
};

class QueryCursor {
 public:
  QueryCursor(Query& query, Node& node) : m_query{query}, m_node{node} {
    m_cursor = ts_query_cursor_new();
    ts_query_cursor_exec(m_cursor, query.query(), node.node());
  }

  ~QueryCursor() { ts_query_cursor_delete(m_cursor); }

  Query& query() { return m_query; }

  bool nextMatch(Match& match) {
    TSQueryMatch _match;
    if (!ts_query_cursor_next_match(m_cursor, &_match)) {
      return false;
    }

    match = Match{*this, _match};

    return true;
  }

  Node node() { return m_node; }

 private:
  Query& m_query;
  Node m_node;
  TSQueryCursor* m_cursor{nullptr};
};

inline Capture Match::capture(const std::string& name) {
  for (uint32_t i = 0; i < m_match.capture_count; i++) {
    const TSQueryCapture& cap = m_match.captures[i];
    uint32_t caplen;
    const char* capname = ts_query_capture_name_for_id(
        m_queryCursor->query().query(), cap.index, &caplen);
    if (capname == name) {
      return Capture{*this, cap};
    }
  }
}

inline Capture Match::operator[](const std::string& name) {
  return capture(name);
}

inline Capture::Capture(Match& match, const TSQueryCapture& capture)
    : m_match{match}, m_capture{capture} {}

inline Node Capture::node() {
  return Node{m_match.queryCursor().node().tree(), m_capture.node};
}
}  // namespace cppts