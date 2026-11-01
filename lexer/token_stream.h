#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "token.h"

namespace lexer {

class TokenStream {
public:
    TokenStream(std::filesystem::path fileName = "unknown_file");
    TokenStream(std::vector<Token> tokens, std::filesystem::path fileName);

    const std::filesystem::path& getFileName() const;
    const Token& peekToken() const;
    void movePos();
    bool atEnd() const;
    bool isMatchType(TokenType type) const;

    const Token& expectToken(TokenType type, const std::string& msg);
    std::int64_t expectInt(const std::string& msg);
    const std::string& expectIdent(const std::string& msg);
    double expectDouble(const std::string& msg);

    [[noreturn]] void failAtToken(const Token& token, const std::string& msg) const;
    void verify() const;

private:
    std::vector<Token> tokens_;
    std::filesystem::path fileName_;
    std::size_t pos_ = 0;
};

} // namespace lexer
