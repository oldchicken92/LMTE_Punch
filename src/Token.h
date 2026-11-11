#pragma once
#include <string>
// tokens can be any text or file structure such as
// a character, number, boolean, file, folder, function, parentheses, comma, operators, newlines

// they have 3 properties: type (TokenType), text (string) and line (int)
enum class TokenType {
    Variable, Type, Number, String, Boolean, File,
    Folder, Function, Keyword, LParen, RParen, LBracket,
    RBracket, Comma, Equals, Newline, EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
    int line;
};

std::string tokenTypeName(TokenType t);