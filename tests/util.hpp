#pragma once

#include <filesystem>
#include <fstream>
#include <string>

inline std::filesystem::path test_file_path(const std::filesystem::path& fn) {
  static const std::filesystem::path test_dir =
      std::filesystem::path{__FILE__}.parent_path();
  return test_dir / fn;
}

inline std::string load_file(const std::filesystem::path& fn) {
  std::stringstream ss;
  std::ifstream ifs{test_file_path(fn)};
  ifs.exceptions(std::ifstream::failbit);
  ss << ifs.rdbuf();
  return ss.str();
}
