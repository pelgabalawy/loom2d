#include <gtest/gtest.h>
#include "text/font.hpp"

#include <string>
#include <vector>

using namespace loom;

// Font::wrap_lines is pure (no GPU): it greedily breaks a string into [start,end)
// byte ranges given each character's advance width. These tests exercise it
// without baking an atlas (which would need a GL context).

namespace {
// Uniform advance per character: width-in-chars math is easy to reason about.
std::vector<float> uniform_advances(const std::string& s, float adv) {
    return std::vector<float>(s.size(), adv);
}
} // namespace

TEST(WrapLines, NoWrapWhenWidthZero) {
    std::string s = "hello world";
    auto lines = Font::wrap_lines(s, uniform_advances(s, 10.f), 0.f);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0].first, 0u);
    EXPECT_EQ(lines[0].second, s.size());
}

TEST(WrapLines, ExplicitNewlineBreaks) {
    std::string s = "ab\ncd";
    auto lines = Font::wrap_lines(s, uniform_advances(s, 10.f), 0.f);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(s.substr(lines[0].first, lines[0].second - lines[0].first), "ab");
    EXPECT_EQ(s.substr(lines[1].first, lines[1].second - lines[1].first), "cd");
}

TEST(WrapLines, BreaksAtSpaceWhenOverflowing) {
    // Each char is 10px wide; max 55px fits "hello" (50) but not "hello " + more.
    std::string s = "hello world";
    auto lines = Font::wrap_lines(s, uniform_advances(s, 10.f), 55.f);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(s.substr(lines[0].first, lines[0].second - lines[0].first), "hello");
    EXPECT_EQ(s.substr(lines[1].first, lines[1].second - lines[1].first), "world");
}

TEST(WrapLines, LongWordWithoutSpaceIsNotBroken) {
    // No space to break on -> the word overflows on a single line.
    std::string s = "supercalifragilistic";
    auto lines = Font::wrap_lines(s, uniform_advances(s, 10.f), 30.f);
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0].second, s.size());
}

TEST(WrapLines, WrapsMultipleTimes) {
    std::string s = "aa bb cc dd";
    // 25px fits one 2-char word (20) plus part; forces a break after each word.
    auto lines = Font::wrap_lines(s, uniform_advances(s, 10.f), 25.f);
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(s.substr(lines[0].first, lines[0].second - lines[0].first), "aa");
    EXPECT_EQ(s.substr(lines[3].first, lines[3].second - lines[3].first), "dd");
}

TEST(WrapLines, NewlineInsideWrappedText) {
    std::string s = "aa bb\ncc";
    auto lines = Font::wrap_lines(s, uniform_advances(s, 10.f), 25.f);
    // "aa bb" fits in 50 > 25 so it wraps to "aa"/"bb", then newline -> "cc".
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(s.substr(lines[2].first, lines[2].second - lines[2].first), "cc");
}
