#include "wgsl_reflect/reflect.hpp"

#include "cppts/parser.hpp"
#include "cppts/tree.hpp"
#include <tree_sitter_wgsl.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace wgsl_reflect {
Reflect::Reflect(const std::filesystem::path& source_file) {
  std::stringstream ss;
  std::ifstream ifs{source_file};
  ifs.exceptions(std::ifstream::failbit);
  ss << ifs.rdbuf();
  m_source = ss.str();
  initialize();
}

Reflect::Reflect(const std::string& source) : m_source{source} { initialize(); }

void Reflect::initialize() {
  m_parser = std::make_unique<cppts::Parser>(tree_sitter_wgsl());
  m_tree = std::make_unique<cppts::Tree>(*m_parser, m_source);
}

std::span<const Entry> Reflect::entries() const { return {m_entries}; }

const Entry& Reflect::entry(size_t i) const { return m_entries.at(i); }

void Reflect::parseFunctions() {
  std::string query_string = "(function_declaration) @thefunc";

  auto cursor = m_tree->query(query_string);
  cppts::Match match;
  while (cursor.nextMatch(match)) {
  }
}

Reflect::~Reflect() = default;

Function::Function(cppts::Node node) {
  auto cursor = node.query(R"Q(
      (function_declaration
        (attribute (identifier) @functype)?
        name: (identifier) @funcname)
      )Q");

  auto match = cursor.nextMatch().value();

  name = match["funcname"].node().str();
}
}  // namespace wgsl_reflect