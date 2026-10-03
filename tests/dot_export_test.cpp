#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "DotExport.hpp"
#include "Regex.hpp"

namespace fs = std::filesystem;

TEST(DotExport, WritesDotAndAttemptsPng) {
  const NFA nfa = Regex("a").to_nfa();
  const std::size_t index = 424242;

  try {
    const std::string png_path = export_automaton_png(nfa, index);
    EXPECT_TRUE(fs::exists(png_path));
    EXPECT_TRUE(fs::exists("output/automaton_424242.dot"));
  } catch (const std::runtime_error &error) {
    // Graphviz may be missing in CI; DOT must still be written.
    EXPECT_TRUE(fs::exists("output/automaton_424242.dot"));
    EXPECT_NE(std::string(error.what()).find("Graphviz"), std::string::npos);
  }

  std::ifstream in("output/automaton_424242.dot");
  ASSERT_TRUE(in.good());
  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
  EXPECT_NE(content.find("digraph Automaton"), std::string::npos);
}

TEST(DotExport, FailsWhenDotPathIsDirectory) {
  const NFA nfa = Regex("a").to_nfa();
  const std::size_t index = 424243;
  fs::create_directories("output");
  const fs::path blocker = "output/automaton_424243.dot";
  fs::remove_all(blocker);
  fs::create_directory(blocker);

  EXPECT_THROW(export_automaton_png(nfa, index), std::runtime_error);
  fs::remove_all(blocker);
}

TEST(DotExport, FailsWhenGraphvizMissing) {
  const NFA nfa = Regex("b").to_nfa();
  const char *old_path = std::getenv("PATH");
  const std::string saved = old_path ? old_path : "";
  _putenv_s("PATH", "");

  try {
    EXPECT_THROW(export_automaton_png(nfa, 424244), std::runtime_error);
    EXPECT_TRUE(fs::exists("output/automaton_424244.dot"));
  } catch (...) {
    _putenv_s("PATH", saved.c_str());
    throw;
  }
  _putenv_s("PATH", saved.c_str());
}
