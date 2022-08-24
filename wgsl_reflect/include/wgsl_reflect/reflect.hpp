#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <string>
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

struct Function {
  explicit Function(cppts::Node node);
  std::string name;
};

enum class EntryType { Vertex, Fragment, Compute };

struct Entry {
  EntryType type;
  Function& function;
};

class Reflect {
 public:
  explicit Reflect(const std::filesystem::path& source_file);
  explicit Reflect(const std::string& source);

  [[nodiscard]] const Entry& entry(size_t i) const;
  [[nodiscard]] std::span<const Entry> entries() const;

  ~Reflect();

 private:
  void initialize();

  void parseFunctions();

  std::string m_source;

  std::unique_ptr<cppts::Parser> m_parser{nullptr};
  std::unique_ptr<cppts::Tree> m_tree{nullptr};

  std::vector<Function> m_functions;
  std::vector<Entry> m_entries;
};
}  // namespace wgsl_reflect
