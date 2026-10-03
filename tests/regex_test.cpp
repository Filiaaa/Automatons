#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "Regex.hpp"

TEST(RegexParse, EmptyThrows) {
  EXPECT_THROW(Regex("").to_nfa(), std::invalid_argument);
  EXPECT_THROW(Regex("   ").to_nfa(), std::invalid_argument);
}

TEST(RegexParse, StripsSpaces) {
  const NFA nfa = Regex(" a + b ").to_nfa();
  EXPECT_TRUE(nfa.accepts("a"));
  EXPECT_TRUE(nfa.accepts("b"));
  EXPECT_FALSE(nfa.accepts(""));
  EXPECT_FALSE(nfa.accepts("ab"));
}

TEST(RegexParse, SymbolConcatAlternationStar) {
  EXPECT_TRUE(Regex("a").to_nfa().accepts("a"));
  EXPECT_TRUE(Regex("ab").to_nfa().accepts("ab"));
  EXPECT_FALSE(Regex("ab").to_nfa().accepts("a"));
  EXPECT_TRUE(Regex("a+b").to_nfa().accepts("a"));
  EXPECT_TRUE(Regex("a+b").to_nfa().accepts("b"));
  EXPECT_TRUE(Regex("a*").to_nfa().accepts(""));
  EXPECT_TRUE(Regex("a*").to_nfa().accepts("aaa"));
  EXPECT_TRUE(Regex("a**").to_nfa().accepts("aa"));
}

TEST(RegexParse, ParenthesesAndPrecedence) {
  const NFA nfa = Regex("a+bc*").to_nfa();
  EXPECT_TRUE(nfa.accepts("a"));
  EXPECT_TRUE(nfa.accepts("b"));
  EXPECT_TRUE(nfa.accepts("bc"));
  EXPECT_TRUE(nfa.accepts("bcc"));
  EXPECT_FALSE(nfa.accepts("ac"));
  EXPECT_FALSE(nfa.accepts(""));

  const NFA grouped = Regex("(a+b)c").to_nfa();
  EXPECT_TRUE(grouped.accepts("ac"));
  EXPECT_TRUE(grouped.accepts("bc"));
  EXPECT_FALSE(grouped.accepts("a"));
}

TEST(RegexParse, EpsilonAtomOne) {
  const NFA eps = Regex("1").to_nfa();
  EXPECT_TRUE(eps.accepts(""));
  EXPECT_FALSE(eps.accepts("1"));

  const NFA mixed = Regex("a+1").to_nfa();
  EXPECT_TRUE(mixed.accepts(""));
  EXPECT_TRUE(mixed.accepts("a"));
  EXPECT_FALSE(mixed.accepts("aa"));

  const NFA concat = Regex("a1b").to_nfa();
  EXPECT_TRUE(concat.accepts("ab"));
  EXPECT_FALSE(concat.accepts("a1b"));
}

TEST(RegexParse, ClassicAbb) {
  const NFA nfa = Regex("(a+b)*abb").to_nfa().minimize();
  EXPECT_TRUE(nfa.accepts("abb"));
  EXPECT_TRUE(nfa.accepts("aabb"));
  EXPECT_TRUE(nfa.accepts("babb"));
  EXPECT_FALSE(nfa.accepts("ab"));
  EXPECT_FALSE(nfa.accepts(""));
  EXPECT_FALSE(nfa.accepts("abba"));
}

TEST(RegexParse, UnexpectedEndAfterPlus) {
  EXPECT_THROW(Regex("a+").to_nfa(), std::invalid_argument);
}

TEST(RegexParse, LeadingStar) {
  EXPECT_THROW(Regex("*a").to_nfa(), std::invalid_argument);
}

TEST(RegexParse, UnclosedParen) {
  EXPECT_THROW(Regex("(a+b").to_nfa(), std::invalid_argument);
}

TEST(RegexParse, UnexpectedTrailing) {
  // ')' after a complete expression is unexpected residual input
  EXPECT_THROW(Regex("a)").to_nfa(), std::invalid_argument);
}

TEST(RegexParse, DuplicateLettersInAlphabetOk) {
  const NFA nfa = Regex("aa").to_nfa();
  EXPECT_EQ(nfa.alphabet().size(), 1u);
  EXPECT_TRUE(nfa.accepts("aa"));
}

TEST(RegexMinimize, EquivalentStarExpressions) {
  const NFA a_star = Regex("a*").to_nfa().minimize();
  const NFA a_star_star = Regex("(a*)*").to_nfa().minimize();
  EXPECT_EQ(a_star.state_count(), a_star_star.state_count());
  EXPECT_TRUE(a_star.accepts(""));
  EXPECT_TRUE(a_star_star.accepts("aaa"));
}

TEST(RegexParse, UnexpectedOperatorAsAtom) {
  EXPECT_THROW(Regex("+a").to_nfa(), std::invalid_argument);
  EXPECT_THROW(Regex(")").to_nfa(), std::invalid_argument);
}

TEST(RegexComplement, ViaRegex) {
  const NFA complement = Regex("a+b").to_nfa().complement();
  EXPECT_TRUE(complement.accepts(""));
  EXPECT_FALSE(complement.accepts("a"));
  EXPECT_FALSE(complement.accepts("b"));
  EXPECT_TRUE(complement.accepts("aa"));
}
