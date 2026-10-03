#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

class NFA {
public:
  /*==================== Helper Structures =====================*/
  struct Edge {
    std::size_t to;
    bool is_epsilon;
    char symbol;
  };

public:
  /*======================= Constructors =======================*/
  NFA(std::size_t state_count, std::size_t start_state,
      const std::vector<bool> &is_accept, const std::vector<char> &alphabet,
      const std::vector<std::vector<Edge>> &edges) {
    validate_start_state(start_state, state_count);
    validate_is_accept(is_accept, state_count);
    validate_alphabet(alphabet);
    validate_edges(edges, state_count);
    state_count_ = state_count;
    start_state_ = start_state;
    is_accept_ = is_accept;
    alphabet_ = alphabet;
    edges_ = edges;
  }

  NFA(const NFA &other) = default;
  NFA(NFA &&other) = default;
  NFA &operator=(const NFA &other) = default;
  NFA &operator=(NFA &&other) = default;
  ~NFA() = default;

private:
  using Subset = std::vector<std::size_t>;

public:
  /*========================= Methods ==========================*/

  std::vector<std::size_t>
  get_epsilon_closure(const std::vector<std::size_t> &sources) const {
    std::vector<std::size_t> result;
    std::queue<std::size_t> queue;
    for (std::size_t source : sources) {
      queue.push(source);
    }
    std::unordered_set<std::size_t> visited;
    while (!queue.empty()) {
      std::size_t current = queue.front();
      queue.pop();
      if (visited.count(current)) {
        continue;
      }
      visited.insert(current);
      result.push_back(current);
      for (const auto &edge : edges_[current]) {
        if (edge.is_epsilon) {
          if (!visited.count(edge.to)) {
            queue.push(edge.to);
          }
        }
      }
    }

    std::sort(result.begin(), result.end());
    return result;
  }

  std::vector<std::size_t> move(const std::vector<std::size_t> &states,
                                char symbol) const {
    std::vector<std::size_t> result;
    for (std::size_t state : states) {
      for (const auto &edge : edges_[state]) {
        if (!edge.is_epsilon && edge.symbol == symbol) {
          result.push_back(edge.to);
        }
      }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
  }

  NFA determinize() const {
    std::map<Subset, std::size_t> subset_indexes;

    std::vector<Subset> subsets;

    std::queue<std::size_t> unprocessed_states;

    Subset start_subset = initial_subset();

    dfa_state_for_subset(start_subset, subset_indexes, subsets,
                         unprocessed_states);

    std::vector<bool> dfa_accept_states;
    std::vector<std::vector<Edge>> dfa_edges;

    while (!unprocessed_states.empty()) {
      std::size_t dfa_state = unprocessed_states.front();
      unprocessed_states.pop();

      dfa_accept_states.push_back(subset_accepts(subsets[dfa_state]));
      dfa_edges.emplace_back();

      for (char symbol : alphabet_) {
        Subset target_subset = subset_on_symbol(subsets[dfa_state], symbol);
        if (target_subset.empty()) {
          continue;
        }

        std::size_t target_state = dfa_state_for_subset(
            target_subset, subset_indexes, subsets, unprocessed_states);
        dfa_edges[dfa_state].push_back(Edge{target_state, false, symbol});
      }
    }

    return NFA(subsets.size(), 0, dfa_accept_states, alphabet_, dfa_edges);
  }

  NFA complete() const {
    if (is_complete()) {
      return *this;
    }

    NFA result = *this;

    std::size_t sink = state_count_;
    result.is_accept_.push_back(false);
    result.edges_.push_back({});
    for (char symbol : alphabet_) {
      result.edges_[sink].push_back(Edge{sink, false, symbol});
    }

    for (std::size_t state = 0; state < state_count_; ++state) {
      for (char symbol : alphabet_) {
        if (move({state}, symbol).empty()) {
          result.edges_[state].push_back(Edge{sink, false, symbol});
        }
      }
    }

    result.state_count_ = state_count_ + 1;
    return result;
  }

  bool is_complete() const {
    for (char symbol : alphabet_) {
      for (std::size_t state = 0; state < state_count_; ++state) {
        if (move({state}, symbol).empty()) {
          return false;
        }
      }
    }

    return true;
  }

  NFA reverse() const {
    std::vector<std::vector<Edge>> reversed_edges(state_count_);
    for (std::size_t state = 0; state < state_count_; ++state) {
      for (const auto &edge : edges_[state]) {
        reversed_edges[edge.to].push_back(
            Edge{state, edge.is_epsilon, edge.symbol});
      }
    }

    std::size_t new_start = state_count_;
    reversed_edges.push_back({});
    for (std::size_t state = 0; state < state_count_; ++state) {
      if (is_accept_[state]) {
        reversed_edges[new_start].push_back(Edge{state, true, '\0'});
      }
    }

    std::vector<bool> reversed_accept(state_count_ + 1, false);
    reversed_accept[start_state_] = true;

    return NFA(state_count_ + 1, new_start, reversed_accept, alphabet_,
               reversed_edges);
  }

  NFA minimize() const {
    return reverse().determinize().reverse().determinize().complete();
  }

  NFA epsilon_closure_nfa() const {
    std::vector<std::vector<Edge>> new_edges(state_count_);
    std::vector<bool> new_accept(state_count_, false);

    for (std::size_t state = 0; state < state_count_; ++state) {
      Subset closure = get_epsilon_closure({state});
      new_accept[state] = subset_accepts(closure);

      for (char symbol : alphabet_) {
        Subset targets = get_epsilon_closure(move(closure, symbol));
        for (std::size_t target : targets) {
          new_edges[state].push_back(Edge{target, false, symbol});
        }
      }
    }

    return NFA(state_count_, start_state_, new_accept, alphabet_, new_edges);
  }

  NFA complement() const {
    NFA result = determinize().complete();
    for (std::size_t state = 0; state < result.state_count_; ++state) {
      result.is_accept_[state] = !result.is_accept_[state];
    }
    return result;
  }

  bool accepts(const std::string &word) const {
    Subset current = get_epsilon_closure({start_state_});
    for (char symbol : word) {
      current = get_epsilon_closure(move(current, symbol));
    }
    return subset_accepts(current);
  }

  std::string to_dot() const {
    std::ostringstream out;
    out << "digraph Automaton {\n";
    out << "  rankdir=LR;\n";
    out << "  node [shape=circle];\n";
    out << "  start_point [shape=point, width=0];\n";
    out << "  start_point -> " << start_state_ << ";\n";

    for (std::size_t state = 0; state < state_count_; ++state) {
      if (is_accept_[state]) {
        out << "  " << state << " [shape=doublecircle];\n";
      }
    }

    for (std::size_t state = 0; state < state_count_; ++state) {
      for (const auto &edge : edges_[state]) {
        out << "  " << state << " -> " << edge.to << " [label=\"";
        if (edge.is_epsilon) {
          out << "ε";
        } else {
          out << edge.symbol;
        }
        out << "\"];\n";
      }
    }

    out << "}\n";
    return out.str();
  }

  std::size_t state_count() const { return state_count_; }
  std::size_t start_state() const { return start_state_; }
  const std::vector<char> &alphabet() const { return alphabet_; }
  const std::vector<bool> &is_accept() const { return is_accept_; }
  const std::vector<std::vector<Edge>> &edges() const { return edges_; }

private:
  /*================= Determinization Helpers =================*/

  bool subset_accepts(const Subset &nfa_states) const {
    for (std::size_t nfa_state : nfa_states) {
      if (is_accept_[nfa_state]) {
        return true;
      }
    }
    return false;
  }

  Subset subset_on_symbol(const Subset &nfa_states, char symbol) const {
    return get_epsilon_closure(move(nfa_states, symbol));
  }

  Subset initial_subset() const {
    Subset subset = get_epsilon_closure({start_state_});
    if (is_accept_[start_state_]) {
      return subset;
    }
    for (const auto &edge : edges_[start_state_]) {
      if (!edge.is_epsilon) {
        return subset;
      }
    }
    subset.erase(std::remove(subset.begin(), subset.end(), start_state_),
                 subset.end());
    return subset;
  }

  std::size_t
  dfa_state_for_subset(const Subset &nfa_states,
                       std::map<Subset, std::size_t> &subset_indexes,
                       std::vector<Subset> &subsets,
                       std::queue<std::size_t> &unprocessed_states) const {
    auto existing = subset_indexes.find(nfa_states);
    if (existing != subset_indexes.end()) {
      return existing->second;
    }

    std::size_t dfa_state = subsets.size();
    subset_indexes.emplace(nfa_states, dfa_state);
    subsets.push_back(nfa_states);
    unprocessed_states.push(dfa_state);
    return dfa_state;
  }

private:
  /*==================== Validation Methods ====================*/
  void validate_start_state(std::size_t start_state,
                            std::size_t state_count) const {
    if (start_state >= state_count) {
      throw std::invalid_argument("Start state is out of range");
    }
  }

  void validate_is_accept(const std::vector<bool> &is_accept,
                          std::size_t state_count) const {
    if (is_accept.size() != state_count) {
      throw std::invalid_argument("Is accept vector is out of range");
    }
  }

  void validate_alphabet(const std::vector<char> &alphabet) const {
    std::set<char> alphabet_set(alphabet.begin(), alphabet.end());
    if (alphabet_set.size() != alphabet.size()) {
      throw std::invalid_argument("Alphabet contains duplicates");
    }
  }

  void validate_edges(const std::vector<std::vector<Edge>> &edges,
                      std::size_t state_count) const {
    if (edges.size() != state_count) {
      throw std::invalid_argument("Edges vector is out of range");
    }
    for (const auto &row : edges) {
      for (const auto &edge : row) {
        if (edge.to >= state_count) {
          throw std::invalid_argument("Edge to state is out of range");
        }
      }
    }
  }

private:
  /*========================== Fields ==========================*/

  std::size_t state_count_;
  std::size_t start_state_;
  std::vector<bool> is_accept_;
  std::vector<char> alphabet_;
  std::vector<std::vector<Edge>> edges_;
};