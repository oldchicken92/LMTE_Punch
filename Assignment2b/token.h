/*
Defines TokenType, the list of every kind of token the lexer can produce 
(Variable, Type, Number, String, Boolean, File, Folder, Function, Keyword, punctuation, 
Newline and EndOfFile). It also defines the Token struct, which holds a token’s type, 
its text and its line number. The lexer produces tokens and the parser consumes them.
*/

#pragma once
#include <string>

enum class TokenType {
    Variable, Type, Number, String, Boolean, File, Folder,
    Function,  Keyword, LParen, RParen, LBracket, RBracket, Comma, Equals,
    Newline, EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
    int line;
};

std::string tokenTypeName(TokenType t);
