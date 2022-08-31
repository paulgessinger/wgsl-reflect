#include "wgsl_reflect/reflect.hpp"

#include "cppts/parser.hpp"
#include "cppts/tree.hpp"
#include <tree_sitter_wgsl.h>

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std::string_literals;

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

  parseFunctions();
  parseEntrypoints();
}

void Reflect::parseFunctions() {
  std::string query_string = "(function_declaration) @thefunc";

  auto cursor = m_tree->query(query_string);
  cppts::Match match;
  while (cursor.nextMatch(match)) {
    Function function{match["thefunc"].node()};
    m_functions.emplace(function.name, std::move(function));
  }
}

void Reflect::parseEntrypoints() {
  std::string query_string = "(function_declaration) @thefunc";
  auto cursor = m_tree->query(query_string);
  cppts::Match match;
  while (cursor.nextMatch(match)) {
    auto func = match["thefunc"].node();
    std::string name{func.child("name").str()};
    //    std::cout << name << std::endl;
    //    std::cout << match["thefunc"].node() << std::endl;
    //    std::cout << match["thefunc"].node().ast() << std::endl;
    //    std::cout << " --- " << std::endl;
    for (auto child : match["thefunc"].node().namedChildren()) {
      if (child.type() != "attribute"s) {
        continue;
      }

      auto it = m_functions.find(name);
      if (it == m_functions.end()) {
        throw std::runtime_error{"Function with name " + name + " not found"};
      }

      std::string type{child.namedChild(0).str()};
      if (type == "vertex") {
        m_entries.vertex.emplace_back(it->second);
      } else if (type == "fragment") {
        m_entries.fragment.emplace_back(it->second);
      } else if (type == "compute") {
        m_entries.compute.emplace_back(it->second);
      }
    }
  }
}

const Function& Reflect::fragment(size_t i) const {
  return m_entries.fragment.at(i);
}

const Function& Reflect::vertex(size_t i) const {
  return m_entries.vertex.at(i);
}

const Function& Reflect::compute(size_t i) const {
  return m_entries.compute.at(i);
}

Reflect::~Reflect() = default;

Function::Function(cppts::Node node) {
  auto cursor = node.query(R"Q(
    (function_declaration
      name: (identifier) @funcname
      (parameter_list)? @params
    ))Q");

  std::cout << node.ast() << std::endl;
  //  return;

  cppts::Match match;
  while (cursor.nextMatch(match)) {
    name = match["funcname"].node().str();

    if (match.has("params")) {
      auto params = match["params"].node();
      FunctionInput input;
      bool haveName = false;
      for (uint32_t i = 0; i < params.namedChildCount(); i++) {
        auto param = params.namedChild(i).child(0);
        std::cout << " - " << param.type() << std::endl;
        if (param.type() == "attribute"s) {
          //          continue;
        }
        if (param.type() == "variable_identifier_declaration"s) {
          input.name = param.child("name").str();
          input.type = param.child("type").str();
          std::cout << input.name << " -> " << input.type << std::endl;
          haveName = true;
        }
      }
      //      assert(haveName && "Did not find input name");
      std::cout << "haveName: " << haveName << std::endl;
      inputs.push_back(std::move(input));
    }
  }
}
}  // namespace wgsl_reflect