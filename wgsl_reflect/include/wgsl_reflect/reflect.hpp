#pragma once

#include <filesystem>
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

struct Input {
  std::string name;
  std::string location;
  std::string locationType;
  std::string typeName;
};

struct InputAttribute {
  std::string name;
  std::string value;
};

struct FunctionInput {
  std::string name;
  std::string type;
  //  std::unordered_map<std::string, std::string> attributes;
  std::vector<InputAttribute> attributes;
};

struct Function {
  explicit Function(cppts::Node node);
  std::string name;
  std::vector<FunctionInput> inputs;
};

enum class EntryType { Vertex, Fragment, Compute };

// struct Entry {
//   EntryType type;
//   Function& function;
// };

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
