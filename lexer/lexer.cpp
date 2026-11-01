#include "lexer.h"

#include <charconv>
#include <cctype>
#include <cmath>
#include <system_error>
#include <utility>

namespace lexer {
namespace {

bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }

bool isIdent(std::string_view text) {
    if (text.empty()) return false;
    for (const unsigned char c : text) {
        if (!std::isdigit(c) && !std::isalpha(c)) return false;
    }
    return true;
}

bool isInteger(std::string_view text, bool allowSign) {
    std::size_t pos = 0;

    if (allowSign && !text.empty() && (text.front() == '+' || text.front() == '-')) ++pos;
    if (pos == text.size()) return false;
    for (; pos < text.size(); ++pos) {
        if (!isDigit(text[pos])) return false;
    }
    return true;
}

bool isDouble(std::string_view text) {
    std::size_t pos = 0;
    if (pos < text.size() && (text[pos] == '+' || text[pos] == '-')) ++pos;

    bool hasDigits = false;
    while (pos < text.size() && isDigit(text[pos])) {
        hasDigits = true;
        ++pos;
    }
    if (pos < text.size() && text[pos] == '.') {
        ++pos;
        while (pos < text.size() && isDigit(text[pos])) {
            hasDigits = true;
            ++pos;
        }
    }
    if (!hasDigits) return false;

    return pos == text.size();
}

} // namespace

Lexer::Lexer(std::string srcBuf, std::filesystem::path fileName)
    : buf_(std::move(srcBuf)), fileName_(std::move(fileName)) {}

const std::filesystem::path& Lexer::getFileName() const { return fileName_; }

bool Lexer::atEnd(std::size_t pos) const { return pos >= buf_.size(); }

unsigned char Lexer::getSym(std::size_t pos) const {
    return static_cast<unsigned char>(buf_.at(pos));
}

void Lexer::advanceTo(std::size_t dst_pos) {
    while (pos_ < dst_pos && !atEnd(pos_)) {
        if (getSym(pos_) == '\n') {
            ++curLine_;
            curCol_ = 1;
        } else {
            ++curCol_;
        }
        ++pos_;
    }
}

void Lexer::skipSpaces() {
    while (!atEnd(pos_) && std::isspace(getSym(pos_))) advanceTo(pos_ + 1);
}

Token Lexer::readInt(std::string_view text) {
    std::string_view digits = text;

    if (digits.front() == '+') digits.remove_prefix(1);

    std::int64_t value = 0;
    const char* const end = digits.data() + digits.size();
    const auto result = std::from_chars(digits.data(), end, value);

    if (result.ec == std::errc{} && result.ptr == end) return makeIntToken(value);

    if (result.ec == std::errc::result_out_of_range) {
        return readDouble(text);
    }

    return makeErrorToken("It is not INT");
}

Token Lexer::readDouble(std::string_view text) {
    if (text.front() == '+') text.remove_prefix(1);

    double value = 0.0;
    const char* const end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value, std::chars_format::general);

    if (result.ec == std::errc::result_out_of_range || !std::isfinite(value)) {
        return makeErrorToken("DOUBLE is out of range");
    }
    
    if (result.ec != std::errc{} || result.ptr != end) {
        return makeErrorToken("It is not DOUBLE");
    }
    return makeDoubleToken(value);
}

Token Lexer::readToken() {
    std::size_t end = pos_;

    while (!atEnd(end) && !std::isspace(getSym(end))) ++end;

    const std::string_view text = std::string_view(buf_).substr(pos_, end - pos_);

    Token token = {};
    if (isInteger(text, true)) {
        token = readInt(text);
    } else if (isDouble(text)) {
        token = readDouble(text);
    } else if (isIdent(text)) {
        token = makeIdentToken(std::string(text));
    } else {
        token = makeErrorToken("Unknown sym in token: " + std::string(text));
    }

    advanceTo(end);
    return token;
}

Token Lexer::makeErrorToken(std::string message) const {
    return {TokenType::ERROR, std::move(message), curLine_, curCol_};
}
Token Lexer::makeIntToken(std::int64_t value) const {
    return {TokenType::INT, value, curLine_, curCol_};
}
Token Lexer::makeDoubleToken(double value) const {
    return {TokenType::DOUBLE, value, curLine_, curCol_};
}
Token Lexer::makeIdentToken(std::string ident) const {
    return {TokenType::IDENTIFIER, std::move(ident), curLine_, curCol_};
}
Token Lexer::makeEndToken() const { return {TokenType::END, std::int64_t{0}, curLine_, curCol_}; }

TokenStream Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipSpaces();
        if (atEnd(pos_)) break;

        tokens.push_back(readToken());

        if (tokens.back().type == TokenType::ERROR) break;
    }

    tokens.push_back(makeEndToken());
    return TokenStream(std::move(tokens), fileName_);
}

} // namespace lexer
