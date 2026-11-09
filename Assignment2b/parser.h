/*
Declares the Parser class and records the full grammar (EBNF) in its header comment. 
It lists the token cursor functions, one parse... function per grammar rule, 
and the syntax-error helpers.
*/

#pragma once
#include <vector>
#include "ast.h"
#include "token.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parseProgram();       

private:
    std::vector<Token> tokens;
    size_t current = 0;
    const Token& peek(size_t offset = 0) const;
    const Token& advance();
    bool check(TokenType type) const;
    bool checkKeyword(const std::string& word) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string& what);
    ExprPtr parseExpression();
    ExprPtr parseLiteral();
    ExprPtr parseVarRef();
    ExprPtr parseList();
    ExprPtr parseCall();
    std::vector<ExprPtr> parseArguments();
    StmtPtr parseStatement();
    StmtPtr parseDeclaration();
    StmtPtr parseChange();
    StmtPtr parseIf();
    StmtPtr parseLoop();
    StmtPtr parseWhile();
    std::vector<StmtPtr> parseBlock();
    [[noreturn]] void fail(const std::string& message) const;
};
