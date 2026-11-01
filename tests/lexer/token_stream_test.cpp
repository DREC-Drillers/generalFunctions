#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "lexer/token_stream.h"

namespace {

lexer::Token endToken() {
    return {lexer::TokenType::END, std::int64_t{0}, 2, 1};
}

std::vector<lexer::Token> sampleTokens() {
    return {{lexer::TokenType::INT, std::int64_t{42}, 1, 1},
            {lexer::TokenType::DOUBLE, 1.25, 1, 4},
            {lexer::TokenType::IDENTIFIER, std::string("name"), 1, 9},
            endToken()};
}

TEST(TokenStream, PeekAndTypeChecksDoNotConsumeTokens) {
    lexer::TokenStream tokens(sampleTokens(), "scene.txt");
    EXPECT_EQ(tokens.getFileName(), "scene.txt");
    EXPECT_FALSE(tokens.atEnd());
    EXPECT_TRUE(tokens.isMatchType(lexer::TokenType::INT));
    EXPECT_FALSE(tokens.isMatchType(lexer::TokenType::DOUBLE));
    EXPECT_EQ(&tokens.peekToken(), &tokens.peekToken());
    EXPECT_EQ(tokens.expectInt("integer"), 42);
}

TEST(TokenStream, ConsumesExpectedValuesAndKeepsEndStable) {
    lexer::TokenStream tokens(sampleTokens(), "scene.txt");
    EXPECT_EQ(tokens.expectInt("integer"), 42);
    EXPECT_DOUBLE_EQ(tokens.expectDouble("double"), 1.25);
    EXPECT_EQ(tokens.expectIdent("identifier"), "name");
    EXPECT_TRUE(tokens.atEnd());
    const auto* end = &tokens.peekToken();
    EXPECT_EQ(&tokens.expectToken(lexer::TokenType::END, "end"), end);
    tokens.movePos();
    EXPECT_EQ(&tokens.peekToken(), end);
    EXPECT_TRUE(tokens.atEnd());
}

TEST(TokenStream, ConvertsIntegerToDouble) {
    lexer::TokenStream tokens(sampleTokens(), "scene.txt");
    EXPECT_DOUBLE_EQ(tokens.expectDouble("number"), 42.0);
    EXPECT_EQ(tokens.peekToken().type, lexer::TokenType::DOUBLE);
}

TEST(TokenStream, TypeErrorsDoNotConsumeToken) {
    lexer::TokenStream tokens(sampleTokens(), "scene.txt");
    EXPECT_THROW(tokens.expectIdent("identifier"), std::runtime_error);
    EXPECT_THROW(tokens.expectToken(lexer::TokenType::DOUBLE, "double"),
                 std::runtime_error);
    EXPECT_EQ(tokens.expectInt("integer"), 42);
    EXPECT_THROW(tokens.expectInt("integer"), std::runtime_error);
    tokens.movePos();
    EXPECT_THROW(tokens.expectDouble("number"), std::runtime_error);
    EXPECT_EQ(tokens.expectIdent("identifier"), "name");
    EXPECT_THROW(tokens.expectInt("integer"), std::runtime_error);
    EXPECT_THROW(tokens.expectIdent("identifier"), std::runtime_error);
    EXPECT_THROW(tokens.expectDouble("number"), std::runtime_error);
}

TEST(TokenStream, IncludesSourceLocationAndContextInErrors) {
    lexer::TokenStream tokens(sampleTokens(), "scene.txt");
    try {
        (void)tokens.expectIdent("Expected a name");
        FAIL() << "Expected a type mismatch";
    } catch (const std::runtime_error& error) {
        EXPECT_EQ(std::string(error.what()), "scene.txt:1:1: Expected a name");
    }
}

TEST(TokenStream, RejectsEmptyTokenList) {
    EXPECT_THROW((lexer::TokenStream(std::vector<lexer::Token>{}, "bad.txt")),
                 std::invalid_argument);
}

TEST(TokenStream, RejectsMissingAndNonterminalEnd) {
    auto missingEnd = sampleTokens();
    missingEnd.pop_back();
    EXPECT_THROW((lexer::TokenStream(missingEnd, "bad.txt")), std::runtime_error);
    EXPECT_THROW((lexer::TokenStream({endToken(), endToken()}, "bad.txt")),
                 std::runtime_error);
}

TEST(TokenStream, RejectsMismatchedTokenData) {
    for (const auto type :
         {lexer::TokenType::INT, lexer::TokenType::DOUBLE, lexer::TokenType::END}) {
        EXPECT_THROW(
            (lexer::TokenStream({{type, std::string("wrong"), 1, 1}}, "bad.txt")),
            std::runtime_error);
    }
    EXPECT_THROW(
        (lexer::TokenStream(
            {{lexer::TokenType::IDENTIFIER, std::int64_t{0}, 1, 1}, endToken()},
            "bad.txt")),
        std::runtime_error);
}

TEST(TokenStream, ReportsErrorTokens) {
    try {
        (void)lexer::TokenStream(
            {{lexer::TokenType::ERROR, std::string("Invalid number"), 3, 7},
             endToken()},
            "bad.txt");
        FAIL() << "Expected an ERROR token to fail";
    } catch (const std::runtime_error& error) {
        EXPECT_EQ(std::string(error.what()), "bad.txt:3:7: Invalid number");
    }
}

} // namespace
