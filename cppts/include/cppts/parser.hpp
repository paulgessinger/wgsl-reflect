#pragma once

#include <tree_sitter/api.h>

namespace cppts {

class Parser {
 public:
  explicit Parser(const TSLanguage* language) : m_language{language} {
    m_parser = ts_parser_new();
    ts_parser_set_language(m_parser, m_language);
  }
  ~Parser() { ts_parser_delete(m_parser); }

  TSParser* parser() { return m_parser; }

 private:
  TSParser* m_parser{nullptr};
  const TSLanguage* m_language{nullptr};
};

}  // namespace cppts