#include <cstdint>
#include <variant>

#include "lexer/lexer.h"

int main() {
    auto tokens = lexer::Lexer("42").tokenize();
    if (tokens.peekToken().type != lexer::TokenType::INT ||
        std::get<std::int64_t>(tokens.peekToken().data) != 42) {
        return 1;
    }
    tokens.movePos();
    return tokens.atEnd() ? 0 : 1;
}
