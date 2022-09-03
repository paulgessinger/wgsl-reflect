#pragma once

#include <nlohmann/json_fwd.hpp>

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

class Reflect;

struct InputAttribute {
  std::string name;
  std::string value;
};

void to_json(nlohmann::json& j, const InputAttribute& attribute);
void to_json(nlohmann::json& j, const std::vector<InputAttribute>& attributes);

struct Input {
  std::string name;
  std::string type;
  std::vector<InputAttribute> attributes;
};

void to_json(nlohmann::json& j, const Input& input);
void to_json(nlohmann::json& j, const std::vector<Input>& inputs);

struct Structure {
  explicit Structure(cppts::Node node);

  std::string name;
  std::vector<Input> members;
};

void to_json(nlohmann::json& j, const Structure& structure);

struct Function {
  explicit Function(cppts::Node node,
                    std::function<std::optional<Structure>(const std::string&)>
                        structLookup = {});

  std::optional<std::string_view> attribute(const std::string& name) const;

  std::string name;
  std::vector<Input> inputs;
  std::unordered_map<std::string, std::string> attributes;
};

void to_json(nlohmann::json& j, const Function& function);

struct Binding {
  explicit Binding(cppts::Node node);
  Binding(const Binding& other) = default;

  Binding& operator=(const Binding& other) = default;

  bool operator==(const Binding& other) const = default;

  static constexpr uint32_t UNSET = std::numeric_limits<uint32_t>::max();

  uint32_t binding{UNSET};
  uint32_t group{UNSET};
  std::string name;
  std::string bindingType;
  std::string type;
};

void to_json(nlohmann::json& j, const Binding& binding);

struct BindGroup {
  const auto& bindings() const { return m_bindings; }
  const auto& binding(size_t i) const { return m_bindings.at(i); }

  size_t size() const { return m_bindings.size(); }

  const auto& operator[](size_t i) const { return binding(i); }

 private:
  friend Reflect;
  std::vector<std::optional<Binding>> m_bindings;
};

void to_json(nlohmann::json& j, const BindGroup& bindGroup);

class Reflect {
 public:
  explicit Reflect(const std::filesystem::path& source_file);
  explicit Reflect(const std::string& source);

  const auto& functions() const { return m_functions; }

  const auto& function(const std::string& name) const {
    return m_functions.at(name);
  }

  const auto& structures() const { return m_structures; }
  const auto& structure(const std::string& name) const {
    return m_structures.at(name);
  }

  [[nodiscard]] const auto& entries() const { return m_entries; }
  [[nodiscard]] const Function& fragment(size_t i) const;
  [[nodiscard]] const Function& vertex(size_t i) const;
  [[nodiscard]] const Function& compute(size_t i) const;

  const auto& bindGroups() const { return m_bindGroups; }
  const auto& bindGroup(size_t i) const { return m_bindGroups.at(i); }

  ~Reflect();

 private:
  void initialize();

  void parseStructures();

  void parseFunctions();

  void parseEntrypoints();

  void parseBindGroups();

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

  std::vector<std::optional<BindGroup>> m_bindGroups;
};

void to_json(nlohmann::json& j, const Reflect& reflect);

}  // namespace wgsl_reflect
