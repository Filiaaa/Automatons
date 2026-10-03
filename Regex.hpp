#pragma once

#include <cctype>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "NFA.hpp"

class Regex {
private:
  /*========================== Usings ==========================*/
  using Edge = NFA::Edge;

private:
  /*==================== Helper Structions =====================*/

  struct Fragment {
    std::size_t start;
    std::size_t accept;
  };

public:
  /*======================= Constructors =======================*/
  Regex(const std::string &pattern) : pattern_(strip_spaces(pattern)) {}

public:
  /*========================= Methods ==========================*/

  NFA to_nfa() const {
    if (pattern_.empty()) {
      throw std::invalid_argument("Empty expression");
    }

    std::vector<char> alphabet = get_alphabet();

    std::vector<std::vector<Edge>> edges;
    std::size_t position = 0;

    Fragment result = parse_expression(position, edges);
    if (position != pattern_.size()) {
      throw std::invalid_argument("Unexpected character in expression");
    }

    std::vector<bool> is_accept(edges.size(), false);
    is_accept[result.accept] = true;
    return NFA(edges.size(), result.start, is_accept, alphabet, edges);
  }

private:
  static std::string strip_spaces(const std::string &pattern) {
    std::string result;
    result.reserve(pattern.size());
    for (unsigned char symbol : pattern) {
      if (!std::isspace(symbol)) {
        result.push_back(static_cast<char>(symbol));
      }
    }
    return result;
  }

private:
  /*========================= Methods ==========================*/

  std::vector<char> get_alphabet() const {
    std::set<char> alphabet_set;
    for (char symbol : pattern_) {
      if (is_symbol(symbol)) {
        alphabet_set.insert(symbol);
      }
    }
    return std::vector<char>(alphabet_set.begin(), alphabet_set.end());
  }

  bool is_symbol(char c) const {
    return c != '(' && c != ')' && c != '*' && c != '+' && c != '1';
  }

  bool starts_factor(std::size_t position) const {
    if (position >= pattern_.size()) {
      return false;
    }
    char current = pattern_[position];
    return is_symbol(current) || current == '(' || current == '1';
  }

  Fragment parse_expression(std::size_t &position,
                            std::vector<std::vector<Edge>> &edges) const {
    Fragment left = parse_term(position, edges);
    while (position < pattern_.size() && pattern_[position] == '+') {
      ++position;
      Fragment right = parse_term(position, edges);
      left = process_alternation(left, right, edges);
    }
    return left;
  }

  Fragment parse_term(std::size_t &position,
                      std::vector<std::vector<Edge>> &edges) const {
    Fragment left = parse_factor(position, edges);
    while (starts_factor(position)) {
      Fragment right = parse_factor(position, edges);
      left = process_concatenation(left, right, edges);
    }
    return left;
  }

  Fragment parse_factor(std::size_t &position,
                        std::vector<std::vector<Edge>> &edges) const {
    Fragment atom = parse_atom(position, edges);
    while (position < pattern_.size() && pattern_[position] == '*') {
      ++position;
      atom = process_kleene_star(atom, edges);
    }
    return atom;
  }

  Fragment parse_atom(std::size_t &position,
                      std::vector<std::vector<Edge>> &edges) const {
    if (position >= pattern_.size()) {
      throw std::invalid_argument("Unexpected end of expression");
    }

    char current = pattern_[position];
    if (current == '(') {
      ++position;
      Fragment inner = parse_expression(position, edges);
      if (position >= pattern_.size() || pattern_[position] != ')') {
        throw std::invalid_argument("Expected ')'");
      }
      ++position;
      return inner;
    }

    if (current == '1') {
      ++position;
      return process_epsilon(edges);
    }

    if (is_symbol(current)) {
      ++position;
      return process_symbol(current, edges);
    }

    throw std::invalid_argument("Unexpected character in expression");
  }

  /*======================== Operations ========================*/

  Fragment process_epsilon(std::vector<std::vector<Edge>> &edges) const {
    std::size_t start = edges.size();
    edges.push_back({});
    edges.push_back({});
    edges[start].push_back(Edge{start + 1, true, '\0'});
    return {start, start + 1};
  }

  Fragment process_symbol(char symbol,
                          std::vector<std::vector<Edge>> &edges) const {
    std::size_t start = edges.size();
    edges.push_back({});
    edges.push_back({});
    edges[start].push_back(Edge{start + 1, false, symbol});
    return {start, start + 1};
  }

  Fragment process_concatenation(const Fragment &lhs, const Fragment &rhs,
                                 std::vector<std::vector<Edge>> &edges) const {
    Edge edge = {rhs.start, true, '\0'};
    edges[lhs.accept].push_back(edge);
    return {lhs.start, rhs.accept};
  }

  Fragment process_alternation(const Fragment &lhs, const Fragment &rhs,
                               std::vector<std::vector<Edge>> &edges) const {
    std::size_t start = edges.size();
    edges.push_back({});
    edges.push_back({});
    Edge edge = {lhs.start, true, '\0'};
    edges[start].push_back(edge);
    edge = {rhs.start, true, '\0'};
    edges[start].push_back(edge);

    edges[lhs.accept].push_back(Edge{start + 1, true, '\0'});
    edges[rhs.accept].push_back(Edge{start + 1, true, '\0'});

    return {start, start + 1};
  }

  Fragment process_kleene_star(const Fragment &fragment,
                               std::vector<std::vector<Edge>> &edges) const {
    std::size_t start = edges.size();
    std::size_t accept = start + 1;
    edges.push_back({});
    edges.push_back({});

    edges[start].push_back(Edge{fragment.start, true, '\0'});
    edges[start].push_back(Edge{accept, true, '\0'});
    edges[fragment.accept].push_back(Edge{fragment.start, true, '\0'});
    edges[fragment.accept].push_back(Edge{accept, true, '\0'});

    return {start, accept};
  }

private:
  /*========================== Fields ==========================*/
  std::string pattern_;
};