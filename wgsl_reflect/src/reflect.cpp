#include "wgsl_reflect/reflect.hpp"

#include "cppts/parser.hpp"
#include "cppts/tree.hpp"
#include <tree_sitter_wgsl.h>

#include <nlohmann/json.hpp>

#include <charconv>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

using namespace std::string_literals;
using namespace nlohmann;

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

  parseStructures();
  parseFunctions();
  parseEntrypoints();
  parseBindGroups();
}

void Reflect::parseStructures() {
  std::string query_string = "(struct_declaration) @thestruct";

  auto cursor = m_tree->query(query_string);
  cppts::Match match;
  while (cursor.nextMatch(match)) {
    Structure _struct{match["thestruct"].node()};
    m_structures.emplace(_struct.name, std::move(_struct));
  }
}

void Reflect::parseFunctions() {
  std::string query_string = "(function_declaration) @thefunc";

  auto getStruct = [this](const std::string& s) -> std::optional<Structure> {
    if (auto it = m_structures.find(s); it != m_structures.end()) {
      return it->second;
    }
    return std::nullopt;
  };

  auto cursor = m_tree->query(query_string);
  cppts::Match match;
  while (cursor.nextMatch(match)) {
    Function function{match["thefunc"].node(), getStruct};
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
    for (auto child : func.namedChildren()) {
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

void Reflect::parseBindGroups() {
  auto cursor = m_tree->query("(global_variable_declaration) @thegroup");
  cppts::Match match;
  while (cursor.nextMatch(match)) {
    auto gnode = match["thegroup"].node();
    Binding binding{gnode};
    if (binding.group + 1 > m_bindGroups.size()) {
      m_bindGroups.resize(binding.group + 1, std::nullopt);
    }

    if (!m_bindGroups[binding.group].has_value()) {
      m_bindGroups[binding.group] = BindGroup{};
    }
    auto& group = m_bindGroups[binding.group].value();
    if (binding.binding + 1 > group.m_bindings.size()) {
      group.m_bindings.resize(binding.binding + 1, std::nullopt);
    }

    group.m_bindings[binding.binding] = std::move(binding);
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

namespace {
Input parseInput(cppts::Node node) {
  Input input;
  bool haveName = false;
  for (auto pchild : node.namedChildren()) {
    if (pchild.type() == "variable_identifier_declaration"s) {
      haveName = true;
      input.name = pchild.child("name").str();
      input.type = pchild.child("type").str();
    } else if (pchild.type() == "attribute"s) {
      input.attributes.push_back(
          InputAttribute{std::string{pchild.namedChild(0).str()},
                         std::string{pchild.namedChild(1).str()}});
    }
  }
  assert(haveName && "Did not find input name");
  return input;
}
}  // namespace

Function::Function(
    cppts::Node node,
    std::function<std::optional<Structure>(const std::string&)> structLookup) {
  if (node.type() != "function_declaration"s) {
    throw std::invalid_argument{"Given node is not a function declaration"};
  }
  name = node.child("name").str();

  for (auto child : node.namedChildren()) {
    if (child.type() == "attribute"s) {
      std::string attrib_name{child.namedChild(0).str()};
      std::string value;
      for (auto next = child.namedChild(0).nextSibling(); !next.isNull();
           next = next.nextSibling()) {
        if (next.str() == "("s || next.str() == ")"s) {
          continue;
        }
        value += next.str();
      }
      attributes.emplace(attrib_name, value);
    } else if (child.type() == "parameter_list"s) {
      for (auto param : child.namedChildren()) {
        inputs.push_back(parseInput(param));
        if (!structLookup) {
          continue;
        }
        if (auto _struct = structLookup(inputs.back().type); _struct) {
          inputs.pop_back();
          for (auto member : _struct->members) {
            inputs.push_back(member);
          }
        }
      }
    }
  }
}
std::optional<std::string_view> Function::attribute(
    const std::string& attrib_name) const {
  if (auto it = attributes.find(attrib_name); it != attributes.end()) {
    return it->second;
  }
  return std::nullopt;
}

Structure::Structure(cppts::Node node) {
  if (node.type() != "struct_declaration"s) {
    throw std::invalid_argument{"Given node is not a struct declaration"};
  }

  name = node.child("name").str();

  for (auto child : node.namedChildren()) {
    if (child.type() != "struct_member"s) {
      continue;
    }
    members.push_back(parseInput(child));
  }
}

Binding::Binding(cppts::Node node) {
  if (node.type() != "global_variable_declaration"s) {
    throw std::invalid_argument{
        "Given node is not a global struct declaration"};
  }

  for (auto child : node.namedChildren()) {
    if (child.type() == "attribute"s) {
      std::string identifier{child.namedChild(0).str()};
      if (identifier == "binding") {
        auto vnode = child.namedChild(1);
        if (vnode.type() != "int_literal"s) {
          throw std::domain_error{identifier + " value of type"s +
                                  vnode.type() + " unsupported"};
        }
        binding = std::stoi(std::string{vnode.str()});
      } else if (identifier == "group") {
        auto vnode = child.namedChild(1);
        if (vnode.type() != "int_literal"s) {
          throw std::domain_error{identifier + " value of type"s +
                                  vnode.type() + " unsupported"};
        }
        group = std::stoi(std::string{vnode.str()});
      }
    } else if (child.type() == "variable_declaration"s) {
      if (auto qual = child.firstChildOfType("variable_qualifier"); qual) {
        if (auto address_space = qual->namedChild(0);
            address_space && address_space.type() == "address_space"s) {
          if (address_space.str() == "uniform" ||
              address_space.str() == "storage") {
            bindingType = "buffer";
          } else {
            throw std::domain_error{"Unknown address_space: " +
                                    std::string{address_space.str()}};
          }
        }

        if (qual->namedChildCount() > 1) {
          if (auto access_mode = qual->namedChild(1);
              access_mode && access_mode.type() == "access_mode"s) {
            // @TODO: Do something with this info
          }
        }
      }
      if (auto idecl =
              child.firstChildOfType("variable_identifier_declaration");
          idecl) {
        name = idecl->child("name").str();
        auto tdecl = idecl->child("type");
        if (bindingType ==
            "") {  // no bindingType yet, pick bindingType from type decl
          assert(tdecl.namedChildCount() == 0 &&
                 "Type decl for builtin type expected");
          std::string ptype{tdecl.str()};
          static const std::regex type_regex{"^(\\w+) ?(?:<(\\w+)>)?$"};
          std::smatch match;
          if (!std::regex_match(ptype, match, type_regex)) {
            throw std::domain_error{"Unable to parse type decl: " + ptype};
          }
          bindingType = match[1].str();
          type = bindingType;
        } else {
          auto identifier = tdecl.namedChild(0);
          type = identifier.str();
        }
      }
    }
  }

  assert(binding != UNSET && "Binding was not found");
  assert(group != UNSET && "Group was not found");
  assert(name != "" && "Name was not found");
  assert(bindingType != "" && "bindingType was not found");
  assert(type != "" && "Type was not found");
}

void to_json(json& j, const Reflect& reflect) {
  j["structures"] = json::object();
  for (const auto& [key, structure] : reflect.structures()) {
    j["structures"][key] = structure;
  }

  j["functions"] = json::object();
  for (const auto& [key, func] : reflect.functions()) {
    j["functions"][key] = func;
  }
  j["entries"] = json::object({{"vertex", json::array()},
                               {"fragment", json::array()},
                               {"compute", json::array()}});

  auto entries = [&](const std::string& name, const auto& items) {
    for (const auto& entry : items) {
      const Function& func = entry;
      j["entries"][name].push_back(func.name);
    }
  };
  entries("vertex", reflect.entries().vertex);
  entries("fragment", reflect.entries().fragment);
  entries("compute", reflect.entries().compute);

  j["bindgroups"] = json::array();
  for (const auto& bindGroup : reflect.bindGroups()) {
    if (bindGroup.has_value()) {
      j["bindgroups"].push_back(*bindGroup);
    } else {
      j["bindgroups"].push_back(nullptr);
    }
  }
}

void to_json(json& j, const Structure& structure) {
  j["name"] = structure.name;
  j["members"] = structure.members;
}

void to_json(json& j, const Input& input) {
  j["name"] = input.name;
  j["type"] = input.type;
  j["attributes"] = input.attributes;
}

void to_json(json& j, const std::vector<Input>& inputs) {
  j = json::array();
  for (const auto& input : inputs) {
    j.push_back(input);
  }
}

void to_json(nlohmann::json& j, const InputAttribute& attribute) {
  //  j["name"] = attribute.name;
  if (attribute.name == "location") {
    j = std::stoi(attribute.value);
  } else {
    j = attribute.value;
  }
}

void to_json(nlohmann::json& j, const std::vector<InputAttribute>& attributes) {
  j = json::object();
  for (const auto& attribute : attributes) {
    j[attribute.name] = attribute;
  }
}

void to_json(nlohmann::json& j, const Function& function) {
  j["name"] = function.name;
  j["inputs"] = function.inputs;
  j["attributes"] = json::object();
  for (auto& [key, value] : function.attributes) {
    if (value == "") {
      j["attributes"][key] = true;
    } else {
      j["attributes"][key] = value;
    }
  }
}

void to_json(nlohmann::json& j, const BindGroup& bindGroup) {
  j = json::array();
  for (const auto& binding : bindGroup.bindings()) {
    if (binding.has_value()) {
      j.push_back(*binding);
    } else {
      j.push_back(nullptr);
    }
  }
}

void to_json(nlohmann::json& j, const Binding& binding) {
  j["binding"] = binding.binding;
  j["group"] = binding.group;
  j["name"] = binding.name;
  j["bindingType"] = binding.bindingType;
  j["type"] = binding.type;
}

}  // namespace wgsl_reflect