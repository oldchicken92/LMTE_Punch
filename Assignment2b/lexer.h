/*
Declares the Lexer class. Its public interface is tokenize(), which turns 
source text into a list of tokens. The private functions are the cursor 
(peek, advance), the skippers for spaces, comments and newlines, and one 
read... function for each kind of token.
*/

#pragma once
#include <string>
#include <vector>
#include "token.h"

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t pos = 0;
    int line = 1;
    bool atEnd() const;
    char peek(size_t offset = 0) const;
    char advance();
    Token makeToken(TokenType type, const std::string& text) const;
    void skipSpaces();
    bool isCommentStart() const;
    void skipComment();
    void handleNewline(std::vector<Token>& tokens);
    Token nextToken();
    Token readPunctuation();
    std::string readDigits();
    Token readNumber();
    Token readString();
    Token readDash();
    Token readVariable();
    Token readType();
    Token readWord();
    Token classifyWord(const std::string& word) const;
};
