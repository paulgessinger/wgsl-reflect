#pragma once

#include "cppts/node.hpp"

#include <tree_sitter/api.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace cppts {

class QueryCursor;
class Node;
class Tree;

class Query : public std::enable_shared_from_this<Query> {
 protected:
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

 public:
  static std::shared_ptr<Query> create(const TSLanguage* language,
                                       std::string_view query) {
    return std::shared_ptr<Query>(new Query(language, query));
  }

  std::shared_ptr<Query> ptr() { return shared_from_this(); }

  ~Query() { ts_query_delete(m_query); }

  TSQuery* getQuery() { return m_query; }

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

  std::optional<Capture> maybe_capture(const std::string& name);

  Capture capture(const std::string& name);

  Capture operator[](const std::string& name);

  bool has(const std::string& name);

  QueryCursor& queryCursor() { return *m_queryCursor; }

 private:
  QueryCursor* m_queryCursor{nullptr};
  TSQueryMatch m_match;
};

class QueryCursor {
 public:
  QueryCursor(std::shared_ptr<Query> query, Node& node)
      : m_query{std::move(query)}, m_node{node} {
    m_cursor = ts_query_cursor_new();
    ts_query_cursor_exec(m_cursor, m_query->getQuery(), node.getNode());
  }

  ~QueryCursor() { ts_query_cursor_delete(m_cursor); }

  Query& query() { return *m_query; }

  bool nextMatch(Match& match) {
    if(auto m = nextMatch();m) {
      match = *m;
      return true;
    }

    return false;

  }

  std::optional<Match> nextMatch() {
    TSQueryMatch _match;

    if (!ts_query_cursor_next_match(m_cursor, &_match)) {
      return std::nullopt;
    }

    return Match{*this, _match};
  }

  Node node() { return m_node; }

 private:
  std::shared_ptr<Query> m_query;
  Node m_node;
  TSQueryCursor* m_cursor{nullptr};
};

inline Capture Match::capture(const std::string& name) {
  if (auto c = maybe_capture(name); c) {
    return *c;
  }
  throw std::invalid_argument{"Capture with name " + name + " does not exist"};
}

inline Capture Match::operator[](const std::string& name) {
  return capture(name);
}

inline bool Match::has(const std::string& name) {
  return maybe_capture(name).has_value();
}

inline std::optional<Capture> Match::maybe_capture(const std::string& name) {
  for (uint32_t i = 0; i < m_match.capture_count; i++) {
    const TSQueryCapture& cap = m_match.captures[i];
    uint32_t caplen;
    const char* capname = ts_query_capture_name_for_id(
        m_queryCursor->query().getQuery(), cap.index, &caplen);
    if (capname == name) {
      return Capture{*this, cap};
    }
  }
  return std::nullopt;
}

inline Capture::Capture(Match& match, const TSQueryCapture& capture)
    : m_match{match}, m_capture{capture} {}

inline Node Capture::node() {
  return Node{m_match.queryCursor().node().getTree(), m_capture.node};
}
}  // namespace cppts