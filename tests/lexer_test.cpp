#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <variant>

#include <gtest/gtest.h>

#include "lexer/lexer.h"

namespace {

TEST(Lexer, EmptyInputAndWhitespaceProduceEnd) {
    for (const auto* source : {"", " \t\r\n"}) {
        auto tokens = lexer::Lexer(source).tokenize();
        EXPECT_TRUE(tokens.atEnd());
        EXPECT_EQ(tokens.peekToken().type, lexer::TokenType::END);
    }
}

TEST(Lexer, RecognizesSignedIntegersAndInt64Limits) {
    auto tokens = lexer::Lexer("0 +42 -17 9223372036854775807 -9223372036854775808")
                      .tokenize();
    for (const auto expected : {std::int64_t{0},
                                std::int64_t{42},
                                std::int64_t{-17},
                                std::numeric_limits<std::int64_t>::max(),
                                std::numeric_limits<std::int64_t>::min()}) {
        ASSERT_EQ(tokens.peekToken().type, lexer::TokenType::INT);
        EXPECT_EQ(std::get<std::int64_t>(tokens.peekToken().data), expected);
        tokens.movePos();
    }
    EXPECT_TRUE(tokens.atEnd());
}

TEST(Lexer, RecognizesDecimalsAndPromotesLargeIntegersToDouble) {
    auto tokens = lexer::Lexer("0.5 -.25 +2. 9223372036854775808").tokenize();
    for (const double expected : {0.5, -0.25, 2.0, 9223372036854775808.0}) {
        ASSERT_EQ(tokens.peekToken().type, lexer::TokenType::DOUBLE);
        EXPECT_DOUBLE_EQ(std::get<double>(tokens.peekToken().data), expected);
        tokens.movePos();
    }
    EXPECT_TRUE(tokens.atEnd());
}

TEST(Lexer, RecognizesAlphanumericIdentifiers) {
    auto tokens = lexer::Lexer("name a12 12abc").tokenize();
    for (const auto* expected : {"name", "a12", "12abc"}) {
        ASSERT_EQ(tokens.peekToken().type, lexer::TokenType::IDENTIFIER);
        EXPECT_EQ(std::get<std::string>(tokens.peekToken().data), expected);
        tokens.movePos();
    }
}

TEST(Lexer, TracksFileLineAndColumn) {
    lexer::Lexer input(" \t12\n  -3.5\nname", "source.txt");
    EXPECT_EQ(input.getFileName(), "source.txt");
    auto tokens = input.tokenize();
    EXPECT_EQ(tokens.getFileName(), "source.txt");
    EXPECT_EQ(tokens.peekToken().line, 1U);
    EXPECT_EQ(tokens.peekToken().col, 3U);
    tokens.movePos();
    EXPECT_EQ(tokens.peekToken().line, 2U);
    EXPECT_EQ(tokens.peekToken().col, 3U);
    tokens.movePos();
    EXPECT_EQ(tokens.peekToken().line, 3U);
    EXPECT_EQ(tokens.peekToken().col, 1U);
    tokens.movePos();
    EXPECT_EQ(tokens.peekToken().line, 3U);
    EXPECT_EQ(tokens.peekToken().col, 5U);
}

class InvalidLexerInput : public testing::TestWithParam<const char*> {};

TEST_P(InvalidLexerInput, RejectsMalformedTokens) {
    EXPECT_THROW(lexer::Lexer(GetParam()).tokenize(), std::runtime_error);
}

INSTANTIATE_TEST_SUITE_P(MalformedTokens, InvalidLexerInput,
                         testing::Values("@", "1.2.3", "+", "-", ".", "a_b", "1,2"));

TEST(Lexer, RejectsDoubleOverflow) {
    EXPECT_THROW(lexer::Lexer(std::string(400, '9')).tokenize(), std::runtime_error);
}

TEST(Lexer, ReportsInvalidTokenLocation) {
    try {
        (void)lexer::Lexer("12\n  @", "bad.txt").tokenize();
        FAIL() << "Expected invalid token to fail";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("bad.txt:2:3:"), std::string::npos);
    }
}

} // namespace
