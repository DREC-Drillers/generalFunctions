#include "lexer/lexer.h"

#include <iostream>

int main() {
    auto tokens = lexer::Lexer{"12 -3.5 name"}.tokenize();

    if (tokens.expectInt("integer") != 12) {
        std::cerr << "Unexpected integer token\n";
        return 1;
    }

    if (tokens.expectDouble("double") != -3.5) {
        std::cerr << "Unexpected floating-point token\n";
        return 1;
    }

    if (tokens.expectIdent("identifier") != "name") {
        std::cerr << "Unexpected identifier token\n";
        return 1;
    }

    if (!tokens.atEnd()) {
        std::cerr << "Expected end of token stream\n";
        return 1;
    }

    return 0;
}

