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

struct Struct {
  explicit Struct(cppts::Node node);

  std::string name;
  std::vector<Input> members;
};

struct Function {
  explicit Function(cppts::Node node,
                    std::function<std::optional<Struct>(const std::string&)>
                        structLookup = {});
  std::string name;
  std::vector<Input> inputs;
};

class Reflect {
 public:
  explicit Reflect(const std::filesystem::path& source_file);
  explicit Reflect(const std::string& source);

  //  [[nodiscard]] const Entry& entry(size_t i) const;
  [[nodiscard]] const auto& entries() const { return m_entries; }

  const auto& functions() const { return m_functions; }

  [[nodiscard]] const Function& fragment(size_t i) const;
  [[nodiscard]] const Function& vertex(size_t i) const;
  [[nodiscard]] const Function& compute(size_t i) const;

  ~Reflect();

 private:
  void initialize();

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
};
}  // namespace wgsl_reflect
