#pragma once
// Parser.h  -  recursive descent parser (tokens -> AST)

// This class converts a token stream into the program's AST. The design follows
// the language grammar closely: each method corresponds to one grammar rule and
// recursively calls the subrules it depends on
//
// The rules here are meant to be simple and strict. The parser does not do
// full semantic validation; it mostly checks syntax, blocks, delimiters, and
// expression structure so the analyser can focus on types and meaning.

#include <vector>
#include "Ast.h"
#include "Errors.h"
#include "Token.h"

// Quick reading guide for this class:
//   - the constructor stores the token stream to parse
//   - parseProgram() begins the top-level grammar rule
//   - parseStatement() decides which statement form is next
//   - parseExpression() creates the leaves of the AST such as numbers, variable
//     references, lists, and function calls
//   - expect()/match() provide the strict token checking used by the grammar

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parseProgram();

private:
    std::vector<Token> tokens;
    size_t current = 0;          // index of the next token to read

    // ---- token cursor -----------------------------------------------------
    const Token& peek(size_t offset = 0) const;    // look without consuming
    Token& advance();                               // consume one token
    bool check(TokenType type) const;                // is next token this type?
    bool checkKeyword(const std::string& word) const; // is next token this keyword?
    bool match(TokenType type);                     // consume if it matches
    Token& expect(TokenType type, const std::string& what);

    // ---- expressions ------------------------------------------------------
    ExprPtr parseExpression();
    ExprPtr parseVarRef();
    ExprPtr parseList();
    ExprPtr parseCall();

    // ---- statements -------------------------------------------------------
    StmtPtr parseStatement();
    StmtPtr parseDeclaration();
    StmtPtr parseChange();
    StmtPtr parseIf();
    StmtPtr parseLoop();
    StmtPtr parseWhile();
    StmtPtr parseExprStatement();
    Block parseBlock();

    // ---- syntax-error helpers --------------------------------------------
    bool atBlockEnd() const;
    void expectCloser(const std::string& closer, const std::string& opener, int openLine);
    [[noreturn]] void fail(const std::string& message) const;
    [[noreturn]] void failAt(int line, const std::string& message) const;
    static std::string describe(const Token& t);
};
