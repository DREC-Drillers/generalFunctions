#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include "token.h"
#include "token_stream.h"

namespace lexer {

class Lexer {
public:
    Lexer(std::string srcBuf, std::filesystem::path fileName = "unknown_file");

    const std::filesystem::path& getFileName() const;
    TokenStream tokenize();

private:
    std::string buf_;
    std::filesystem::path fileName_;
    std::size_t pos_ = 0;
    std::size_t curLine_ = 1;
    std::size_t curCol_ = 1;

    Token readToken();
    Token readInt(std::string_view text);
    Token readDouble(std::string_view text);

    bool atEnd(std::size_t pos) const;
    unsigned char getSym(std::size_t pos) const;
    void advanceTo(std::size_t dst_pos);
    void skipSpaces();

    Token makeErrorToken(std::string message) const;
    Token makeIdentToken(std::string ident) const;
    Token makeIntToken(std::int64_t value) const;
    Token makeDoubleToken(double value) const;
    Token makeEndToken() const;
};

} // namespace lexer
