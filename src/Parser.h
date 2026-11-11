#pragma once
/*
Parser.h file is the recursive decent parser which turns tokens to and AST

This file builds a top-down parser that is a syntax analyser. It reads the tokens
produced by the lexer and builds an abstract syntax tree to represent the 
overall programs structure.

Parser flows like this: 
1. parseProgram() starts the grammar at the top level
2. parseBlock() reads a sequence of statements until a closing keyword or end of file  is found.
3. parseStatement() identifies the next statement form using a one-token lookahead
4. parseExpression() handles literal values and nested expressions such a v
ariable indexing and function arguments.
*/

#include <vector>
#include "Ast.h"
#include "Errors.h"
#include "Token.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens); // parser it built around token stream
    std::unique_ptr<Program> parseProgram(); // parses the entire file

private:
    std::vector<Token> tokens; 
    size_t current = 0;          // index of the next token to read

    // token cursor 
    const Token& peek(size_t offset = 0) const;    // look without consuming
    Token& advance();    // consume one token
    bool check(TokenType type) const; // check token typ
    bool checkKeyword(const std::string& word) const; 
    bool match(TokenType type); // consume if it matches
    Token& expect(TokenType type, const std::string& what);

    // expressions, turns token sequence into a ast node
    ExprPtr parseExpression();
    ExprPtr parseVarRef();
    ExprPtr parseList();
    ExprPtr parseCall();

    // statements
    StmtPtr parseStatement();
    StmtPtr parseDeclaration();
    StmtPtr parseChange();
    StmtPtr parseIf();
    StmtPtr parseLoop();
    StmtPtr parseWhile();
    StmtPtr parseExprStatement();
    Block parseBlock();

    // syntax-error helpers
    bool atBlockEnd() const;
    void expectCloser(const std::string& closer, const std::string& opener, int openLine);
    [[noreturn]] void fail(const std::string& message) const;
    [[noreturn]] void failAt(int line, const std::string& message) const;
    static std::string describe(const Token& t);
};
