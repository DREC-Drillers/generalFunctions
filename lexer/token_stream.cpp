#include "token_stream.h"

#include <stdexcept>
#include <utility>
#include <variant>

namespace lexer {

TokenStream::TokenStream(std::filesystem::path fileName)
    : fileName_(std::move(fileName)) {}

TokenStream::TokenStream(std::vector<Token> tokens, std::filesystem::path fileName)
    : tokens_(std::move(tokens)), fileName_(std::move(fileName)) {
    verify();
}

void TokenStream::verify() const {
    if (tokens_.empty()) {
        throw std::invalid_argument("Token list is empty");
    }

    for (std::size_t i = 0; i < tokens_.size(); ++i) {

        const Token& token = tokens_[i];

        if (token.type == TokenType::ERROR) {
            failAtToken(token, std::holds_alternative<std::string>(token.data)
                                   ? std::get<std::string>(token.data)
                                   : "Malformed ERROR token");
        }

        if (token.type == TokenType::END && i + 1 != tokens_.size()) {
            failAtToken(token, "END must be the last token");
        }

        const bool validData =
            ((token.type == TokenType::INT || token.type == TokenType::END) &&
             std::holds_alternative<std::int64_t>(token.data)) ||
            (token.type == TokenType::IDENTIFIER && std::holds_alternative<std::string>(token.data)) ||
            (token.type == TokenType::DOUBLE && std::holds_alternative<double>(token.data));

        if (!validData) {
            failAtToken(token, "Token type does not match its data");
        }
    }

    if (tokens_.back().type != TokenType::END) {
        failAtToken(tokens_.back(), "Token list must end with END");
    }
}

const std::filesystem::path& TokenStream::getFileName() const { return fileName_; }

const Token& TokenStream::peekToken() const { return tokens_.at(pos_); }

void TokenStream::movePos() {
    if (peekToken().type != TokenType::END) ++pos_;
}

bool TokenStream::atEnd() const { return peekToken().type == TokenType::END; }

bool TokenStream::isMatchType(TokenType type) const { return peekToken().type == type; }

const Token& TokenStream::expectToken(TokenType type, const std::string& msg) {
    const Token& token = peekToken();

    if (token.type != type) failAtToken(token, msg);

    movePos();
    return token;
}

std::int64_t TokenStream::expectInt(const std::string& msg) {
    return std::get<std::int64_t>(expectToken(TokenType::INT, msg).data);
}

const std::string& TokenStream::expectIdent(const std::string& msg) {
    return std::get<std::string>(expectToken(TokenType::IDENTIFIER, msg).data);
}

double TokenStream::expectDouble(const std::string& msg) {
    const Token& token = peekToken();

    if (token.type == TokenType::INT) {
        const double value = static_cast<double>(std::get<std::int64_t>(token.data));

        movePos();
        return value;
    }
    if (token.type == TokenType::DOUBLE) {
        const double value = std::get<double>(token.data);

        movePos();
        return value;
    }

    failAtToken(token, msg);
}

[[noreturn]] void TokenStream::failAtToken(const Token& token, const std::string& msg) const {
    throw std::runtime_error(fileName_.string() + ":" + std::to_string(token.line) + ":" +
                             std::to_string(token.col) + ": " + msg);
}

} // namespace lexer
