#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace cppts {
class Parser;
class Tree;
class Node;
}  // namespace cppts

namespace wgsl_reflect {

struct InputAttribute {
  std::string name;
  std::string value;
};

struct Input {
  std::string name;
  std::string type;
  std::vector<InputAttribute> attributes;
};

struct Structure {
  explicit Structure(cppts::Node node);

  std::string name;
  std::vector<Input> members;
};

struct Function {
  explicit Function(cppts::Node node,
                    std::function<std::optional<Structure>(const std::string&)>
                        structLookup = {});

  std::optional<std::string_view> attribute(const std::string& name) const;

  std::string name;
  std::vector<Input> inputs;
  std::unordered_map<std::string, std::string> attributes;
};

class Reflect {
 public:
  explicit Reflect(const std::filesystem::path& source_file);
  explicit Reflect(const std::string& source);

  //  [[nodiscard]] const Entry& entry(size_t i) const;
  [[nodiscard]] const auto& entries() const { return m_entries; }

  const auto& functions() const { return m_functions; }

  const auto& function(const std::string& name) const {
    return m_functions.at(name);
  }

  const auto& structures() const { return m_structures; }
  const auto& structure(const std::string& name) const {
    return m_structures.at(name);
  }

  [[nodiscard]] const Function& fragment(size_t i) const;
  [[nodiscard]] const Function& vertex(size_t i) const;
  [[nodiscard]] const Function& compute(size_t i) const;

  ~Reflect();

 private:
  void initialize();

  void parseStructures();

  void parseFunctions();

  void parseEntrypoints();

  std::string m_source;

  std::unique_ptr<cppts::Parser> m_parser{nullptr};
  std::unique_ptr<cppts::Tree> m_tree{nullptr};

  struct {
    std::vector<std::reference_wrapper<Function>> vertex;
    std::vector<std::reference_wrapper<Function>> fragment;
    std::vector<std::reference_wrapper<Function>> compute;
  } m_entries;

  std::unordered_map<std::string, Function> m_functions;
  std::unordered_map<std::string, Structure> m_structures;
};
}  // namespace wgsl_reflect
