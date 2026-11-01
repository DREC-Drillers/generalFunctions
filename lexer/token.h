#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace lexer {

enum class TokenType {
    INT,
    END,
    ERROR,
    IDENTIFIER,
    DOUBLE
};

struct Token {
    TokenType type;
    std::variant<std::int64_t, std::string, double> data;
    std::size_t line;
    std::size_t col;
};

} // namespace lexer
