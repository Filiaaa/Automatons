#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "NFA.hpp"

namespace fs = std::filesystem;

inline std::string export_automaton_png(const NFA &automaton,
                                        std::size_t draw_index) {
  fs::create_directories("output");

  const std::string base =
      "output/automaton_" + std::to_string(draw_index);
  const std::string dot_path = base + ".dot";
  const std::string png_path = base + ".png";

  std::ofstream out(dot_path);
  if (!out) {
    throw std::runtime_error("Cannot write file: " + dot_path);
  }
  out << automaton.to_dot();
  out.close();

  const std::string command =
      "dot -Tpng \"" + dot_path + "\" -o \"" + png_path + "\"";
  const int code = std::system(command.c_str());
  if (code != 0) {
    throw std::runtime_error(
        "Graphviz failed. Install Graphviz and ensure `dot` is in PATH. "
        "DOT file saved at: " +
        dot_path);
  }

  return png_path;
}
