/*
This file is the lexical scanner for the Punch language. 

It goes through the raw source file one character at a time and turns it into a stream of 
tokens, for the parser to use. Each helper method consumes a specific token form such
as a variable, number, string, keyword, or punctuation.
*/

#pragma once

#include <string>
#include <vector>
#include "Errors.h"
#include "Token.h"

class Lexer { // reads source characters and turns them into a token stream that the parser understands.
public:
    // create a scanner bound to one script string.
    explicit Lexer(const std::string& source);

    // make a token stream, ending at end of file
    // returns vector of the token type
    std::vector<Token> tokenize();

private:
    std::string src;  // the original source text being scanned
    size_t pos = 0;   // reading position in src, index of next unread character
    int line = 1;     // line number used for erros

    // helpers used to go through source file and helps with bound checking
    bool atEnd() const;
    char peek(size_t offset = 0) const;
    char advance();

    // Shared token creation and error reporting
    Token makeToken(TokenType type, const std::string& text) const;
    PunchError error(const std::string& message) const;

    // Whitespace and comments are discarded before the parser sees them.
    void skipSpaces();
    bool isCommentStart() const;
    void skipComment();
    void handleNewline(std::vector<Token>& tokens);

    // The state machine methodfs that recognise each token kind
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
