#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "NFA.hpp"

using Edge = NFA::Edge;

namespace {

NFA make_manual_nfa() {
  // 0 -eps-> 1 -eps-> 2, 1 -eps-> 1, 0 -a-> 3 (accept 2)
  std::vector<std::vector<Edge>> edges(4);
  edges[0].push_back(Edge{1, true, '\0'});
  edges[0].push_back(Edge{3, false, 'a'});
  edges[1].push_back(Edge{2, true, '\0'});
  edges[1].push_back(Edge{1, true, '\0'});
  return NFA(4, 0, {false, false, true, false}, {'a'}, edges);
}

} // namespace

TEST(NFAValidation, RejectsBadStart) {
  EXPECT_THROW(NFA(1, 1, {false}, {}, {{}}), std::invalid_argument);
}

TEST(NFAValidation, RejectsBadAcceptSize) {
  EXPECT_THROW(NFA(1, 0, {false, true}, {}, {{}}), std::invalid_argument);
}

TEST(NFAValidation, RejectsDuplicateAlphabet) {
  EXPECT_THROW(NFA(1, 0, {false}, {'a', 'a'}, {{}}), std::invalid_argument);
}

TEST(NFAValidation, RejectsBadEdgesSize) {
  EXPECT_THROW(NFA(2, 0, {false, false}, {'a'}, {{}}), std::invalid_argument);
}

TEST(NFAValidation, RejectsEdgeOutOfRange) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{5, false, 'a'});
  EXPECT_THROW(NFA(1, 0, {false}, {'a'}, edges), std::invalid_argument);
}

TEST(NFACore, EpsilonClosureWithCycle) {
  const NFA nfa = make_manual_nfa();
  const auto closure = nfa.get_epsilon_closure({0});
  EXPECT_EQ(closure, (std::vector<std::size_t>{0, 1, 2}));
}

TEST(NFACore, EpsilonClosureDuplicateQueueEntries) {
  // 0 -> 1, 0 -> 2, 1 -> 3, 2 -> 3 : state 3 is enqueued twice before visit.
  std::vector<std::vector<Edge>> edges(4);
  edges[0].push_back(Edge{1, true, '\0'});
  edges[0].push_back(Edge{2, true, '\0'});
  edges[1].push_back(Edge{3, true, '\0'});
  edges[2].push_back(Edge{3, true, '\0'});
  const NFA nfa(4, 0, {false, false, false, true}, {}, edges);
  EXPECT_EQ(nfa.get_epsilon_closure({0}),
            (std::vector<std::size_t>{0, 1, 2, 3}));
}

TEST(NFACore, MoveIgnoresEpsilonAndDedups) {
  std::vector<std::vector<Edge>> edges(2);
  edges[0].push_back(Edge{1, false, 'a'});
  edges[0].push_back(Edge{1, false, 'a'});
  edges[0].push_back(Edge{1, true, '\0'});
  const NFA nfa(2, 0, {false, true}, {'a'}, edges);
  EXPECT_EQ(nfa.move({0}, 'a'), (std::vector<std::size_t>{1}));
  EXPECT_TRUE(nfa.move({0}, 'b').empty());
}

TEST(NFACore, CopyMoveAssign) {
  NFA a = make_manual_nfa();
  NFA b = a;
  NFA c(1, 0, {true}, {}, {{}});
  c = a;
  NFA d = std::move(b);
  NFA e(1, 0, {false}, {}, {{}});
  e = std::move(c);
  EXPECT_TRUE(d.accepts(""));
  EXPECT_TRUE(e.accepts(""));
  EXPECT_FALSE(d.accepts("a"));
}

TEST(NFACore, GettersAndToDot) {
  const NFA nfa = make_manual_nfa();
  EXPECT_EQ(nfa.state_count(), 4u);
  EXPECT_EQ(nfa.start_state(), 0u);
  EXPECT_EQ(nfa.alphabet(), (std::vector<char>{'a'}));
  EXPECT_EQ(nfa.is_accept().size(), 4u);
  EXPECT_EQ(nfa.edges().size(), 4u);

  const std::string dot = nfa.to_dot();
  EXPECT_NE(dot.find("digraph Automaton"), std::string::npos);
  EXPECT_NE(dot.find("start_point -> 0"), std::string::npos);
  EXPECT_NE(dot.find("doublecircle"), std::string::npos);
  EXPECT_NE(dot.find("label=\"ε\""), std::string::npos);
  EXPECT_NE(dot.find("label=\"a\""), std::string::npos);
}

TEST(NFAComplete, AlreadyCompleteReturnsSame) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{0, false, 'a'});
  const NFA nfa(1, 0, {true}, {'a'}, edges);
  EXPECT_TRUE(nfa.is_complete());
  const NFA completed = nfa.complete();
  EXPECT_EQ(completed.state_count(), 1u);
}

TEST(NFAComplete, AddsSink) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{0, false, 'a'});
  const NFA nfa(1, 0, {true}, {'a', 'b'}, edges);
  EXPECT_FALSE(nfa.is_complete());
  const NFA completed = nfa.complete();
  EXPECT_EQ(completed.state_count(), 2u);
  EXPECT_TRUE(completed.is_complete());
  EXPECT_FALSE(completed.is_accept()[1]);
}

TEST(NFADeterminize, NondeterministicExample) {
  // 0 -a-> 0, 0 -a-> 1, accept only 1
  std::vector<std::vector<Edge>> edges(2);
  edges[0].push_back(Edge{0, false, 'a'});
  edges[0].push_back(Edge{1, false, 'a'});
  const NFA nfa(2, 0, {false, true}, {'a'}, edges);
  const NFA dfa = nfa.determinize();
  EXPECT_TRUE(dfa.accepts("a"));
  EXPECT_TRUE(dfa.accepts("aa"));
  EXPECT_FALSE(dfa.accepts(""));
}

TEST(NFADeterminize, SkipsEmptySubsetTargets) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{0, false, 'a'});
  const NFA nfa(1, 0, {true}, {'a', 'b'}, edges);
  const NFA dfa = nfa.determinize();
  EXPECT_TRUE(dfa.accepts(""));
  EXPECT_TRUE(dfa.accepts("aaa"));
  EXPECT_FALSE(dfa.accepts("b"));
}

TEST(NFAReverseAndMinimize, AStarIsOneState) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{0, false, 'a'});
  const NFA nfa(1, 0, {true}, {'a'}, edges);
  const NFA minimal = nfa.minimize();
  EXPECT_EQ(minimal.state_count(), 1u);
  EXPECT_TRUE(minimal.accepts(""));
  EXPECT_TRUE(minimal.accepts("aaa"));
}

TEST(NFAReverseAndMinimize, ReverseCreatesAuxStart) {
  std::vector<std::vector<Edge>> edges(2);
  edges[0].push_back(Edge{1, false, 'a'});
  const NFA nfa(2, 0, {false, true}, {'a'}, edges);
  const NFA reversed = nfa.reverse();
  EXPECT_EQ(reversed.state_count(), 3u);
  EXPECT_TRUE(reversed.accepts("a"));
  EXPECT_FALSE(reversed.accepts(""));
}

TEST(NFAEpsilonClosureNfa, RemovesEpsilonEdges) {
  const NFA nfa = make_manual_nfa();
  const NFA no_eps = nfa.epsilon_closure_nfa();
  for (const auto &row : no_eps.edges()) {
    for (const auto &edge : row) {
      EXPECT_FALSE(edge.is_epsilon);
    }
  }
  EXPECT_TRUE(no_eps.accepts(""));
  EXPECT_FALSE(no_eps.accepts("a"));
}

TEST(NFAComplement, ComplementOfAPlusB) {
  // Built as DFA manually for {a,b}
  std::vector<std::vector<Edge>> edges(3);
  edges[0].push_back(Edge{1, false, 'a'});
  edges[0].push_back(Edge{1, false, 'b'});
  edges[1].push_back(Edge{2, false, 'a'});
  edges[1].push_back(Edge{2, false, 'b'});
  edges[2].push_back(Edge{2, false, 'a'});
  edges[2].push_back(Edge{2, false, 'b'});
  const NFA language(3, 0, {false, true, false}, {'a', 'b'}, edges);

  const NFA complement = language.complement();
  EXPECT_TRUE(complement.accepts(""));
  EXPECT_FALSE(complement.accepts("a"));
  EXPECT_FALSE(complement.accepts("b"));
  EXPECT_TRUE(complement.accepts("aa"));
  EXPECT_TRUE(complement.accepts("ab"));
}

TEST(NFAInitialSubset, AcceptingStartKeepsStartInSubset) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{0, true, '\0'});
  const NFA nfa(1, 0, {true}, {}, edges);
  const NFA dfa = nfa.determinize();
  EXPECT_TRUE(dfa.accepts(""));
}

TEST(NFAInitialSubset, NonEpsilonStartKeepsStart) {
  std::vector<std::vector<Edge>> edges(2);
  edges[0].push_back(Edge{1, false, 'a'});
  const NFA nfa(2, 0, {false, true}, {'a'}, edges);
  const NFA dfa = nfa.determinize();
  EXPECT_FALSE(dfa.accepts(""));
  EXPECT_TRUE(dfa.accepts("a"));
}

TEST(NFAAccepts, RejectsUnknownSymbolPath) {
  std::vector<std::vector<Edge>> edges(1);
  edges[0].push_back(Edge{0, false, 'a'});
  const NFA nfa(1, 0, {true}, {'a'}, edges);
  EXPECT_FALSE(nfa.accepts("b"));
}
