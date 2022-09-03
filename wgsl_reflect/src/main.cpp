#include "wgsl_reflect/reflect.hpp"

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  CLI::App app{"App description"};

  std::filesystem::path filename;
  app.add_option("file", filename, "The .wgsl file to process")->required();

  CLI11_PARSE(app, argc, argv);

  wgsl_reflect::Reflect reflect{filename};

  std::cout << filename << std::endl;

  nlohmann::json j;
  j = reflect;

  std::cout << j.dump(2) << std::endl;

  return 0;
}