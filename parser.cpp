#include "Parser.h"
#include <algorithm>

// parser.cpp  -  grammar-driven construction of the AST
//
// The parser reads the token stream produced by the lexer and builds the nested
// tree structure described in the language grammar. Each rule is responsible for
// matching one piece of syntax and then handing control to the next rule.
//
// Reading guide:
//   - parseProgram() is the top-level entry point.
//   - parseStatement() decides which statement form is next.
//   - parseExpression() handles literals, variables, lists, and calls.
//   - expect()/match() keep the parser strict and predictable.
//
// The whole grammar is deliberately line-oriented: one statement per line, with
// a matching block closer such as endif/endloop/endwhile. That makes the AST
// easier to reason about and gives later passes a clear structure to validate.

Parser::Parser(std::vector<Token> t) : tokens(std::move(t)) {}

// ============================================================================
// TOKEN CURSOR
// All reading goes through these few functions, so bounds are checked once.
// ============================================================================

// Look at a token without consuming it. Reading past the end keeps returning
// the final EOF token, so the parser can never run off the array.
const Token& Parser::peek(size_t offset) const {
    size_t i = std::min(current + offset, tokens.size() - 1);
    return tokens[i];
}

// Consume and return the next token (never moves past EOF).
Token& Parser::advance() {
    const Token& t = peek();
    if (current < tokens.size() - 1) current++;
    return const_cast<Token&>(t);
}

bool Parser::check(TokenType type) const { return peek().type == type; }

bool Parser::checkKeyword(const std::string& word) const {
    return peek().type == TokenType::Keyword && peek().text == word;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

// Consume a token of the given type or report a syntax error naming what we
// wanted and what we actually found.
Token& Parser::expect(TokenType type, const std::string& what) {
    if (!check(type)) fail("expected " + what + " but found " + describe(peek()));
    return advance();
}

// SYNTAX-ERROR HELPERS  (Part D: syntactic analysis)

// Human-readable description of a token for error messages.
std::string Parser::describe(const Token& t) {
    switch (t.type) {
        case TokenType::EndOfFile: return "end of file";
        case TokenType::Newline:   return "end of line";
        case TokenType::Variable:  return "variable '-" + t.text + "-'";
        case TokenType::String:    return "string \"" + t.text + "\"";
        default:                   return "'" + t.text + "'";
    }
}

void Parser::failAt(int line, const std::string& message) const {
    throw PunchError("syntax", line, message);
}
void Parser::fail(const std::string& message) const { failAt(peek().line, message); }

// Words that end a block. Statements never start with these, so parseBlock
// stops when it sees one and lets the opener decide whether it is correct.
bool Parser::atBlockEnd() const {
    if (check(TokenType::EndOfFile)) return true;
    return checkKeyword("else") || checkKeyword("endif") ||
           checkKeyword("endloop") || checkKeyword("endwhile");
}

// After a block, require the closer that matches the opener.
// Reports the three classic block mistakes with useful messages:
//   - missing closer (hit end of file)
//   - wrong closer  (e.g. an `if` closed by `endloop`)
void Parser::expectCloser(const std::string& closer, const std::string& opener, int openLine) {
    if (checkKeyword(closer)) { advance(); return; }
    if (check(TokenType::EndOfFile))
        fail("missing '" + closer + "' for the '" + opener + "' on line " + std::to_string(openLine));
    fail("expected '" + closer + "' to close the '" + opener + "' on line " +
         std::to_string(openLine) + " but found " + describe(peek()));
}

// ============================================================================
// PROGRAM AND BLOCKS
// ============================================================================

// program = block ;   then nothing may be left over.
std::unique_ptr<Program> Parser::parseProgram() {
    auto program = std::make_unique<Program>();
    program->statements = parseBlock();
    // parseBlock stops early at else/endif/... A leftover one at top level
    // has no opener: a stray closer.
    if (!check(TokenType::EndOfFile))
        fail(describe(peek()) + " has no matching block to close");
    return program;
}

// block = { statement NEWLINE } ;
// Each statement must sit alone on its line (one statement per line rule).
Block Parser::parseBlock() {
    Block statements;
    while (!atBlockEnd()) {
        statements.push_back(parseStatement());
        expect(TokenType::Newline, "end of line after the statement");
    }
    return statements;
}

// ============================================================================
// STATEMENTS
// The first token tells us which kind of statement this is (one-token lookahead).
// ============================================================================
StmtPtr Parser::parseStatement() {
    switch (peek().type) {
        case TokenType::Variable: return parseDeclaration();
        case TokenType::Function: return parseExprStatement();
        case TokenType::Keyword:
            if (checkKeyword("change")) return parseChange();
            if (checkKeyword("if"))     return parseIf();
            if (checkKeyword("loop"))   return parseLoop();
            if (checkKeyword("while"))  return parseWhile();
            break;
        default: break;
    }
    fail("a statement cannot start with " + describe(peek()));
}

// declaration = VARIABLE TYPE "=" expression ;      e.g.  -num-number = 10
StmtPtr Parser::parseDeclaration() {
    auto node = std::make_unique<Declaration>();
    const Token& var = advance();
    node->line = var.line;
    node->name = var.text;

    // A variable at the start of a line must be a declaration, so a TYPE
    // must follow. `-x- = 5` is the common mistake; point at change().
    if (!check(TokenType::Type))
        fail("'-" + var.text + "-' needs a type to be declared, e.g. -" + var.text +
             "-number = 5 (use change(-" + var.text + "-, value) to update it)");
    Token typeToken = advance();
    parseTypeName(typeToken.text, node->type);    // lexer already validated the word

    expect(TokenType::Equals, "'=' after the type");
    node->value = parseExpression();
    return node;
}

// change_stmt = "change" "(" target "," expression ")" ;
// A special form: the first argument is a variable REFERENCE (something to
// overwrite), not a value, so it has its own rule instead of using parseCall.
StmtPtr Parser::parseChange() {
    auto node = std::make_unique<Change>();
    node->line = advance().line;                       // 'change'
    expect(TokenType::LParen, "'(' after 'change'");
    if (!check(TokenType::Variable))
        fail("change() needs a variable first, found " + describe(peek()));
    node->target.reset(static_cast<VarRef*>(parseVarRef().release()));
    expect(TokenType::Comma, "',' between the variable and the new value");
    node->value = parseExpression();
    expect(TokenType::RParen, "')' to close change(");
    return node;
}

// if_stmt = "if" "(" expression ")" NEWLINE block [ "else" NEWLINE block ] "endif" ;
StmtPtr Parser::parseIf() {
    auto node = std::make_unique<If>();
    node->line = advance().line;                       // 'if'
    expect(TokenType::LParen, "'(' after 'if'");
    node->condition = parseExpression();
    expect(TokenType::RParen, "')' to close the condition");
    expect(TokenType::Newline, "end of line after if(...)");
    node->thenBlock = parseBlock();
    if (checkKeyword("else")) {
        advance();
        expect(TokenType::Newline, "end of line after 'else'");
        node->elseBlock = parseBlock();
    }
    expectCloser("endif", "if", node->line);
    return node;
}

// loop_stmt = "loop" "(" expression ")" NEWLINE block "endloop" ;
StmtPtr Parser::parseLoop() {
    auto node = std::make_unique<Loop>();
    node->line = advance().line;                       // 'loop'
    expect(TokenType::LParen, "'(' after 'loop'");
    node->count = parseExpression();
    expect(TokenType::RParen, "')' to close the loop count");
    expect(TokenType::Newline, "end of line after loop(...)");
    node->body = parseBlock();
    expectCloser("endloop", "loop", node->line);
    return node;
}

// while_stmt = "while" "(" expression ")" NEWLINE block "endwhile" ;
StmtPtr Parser::parseWhile() {
    auto node = std::make_unique<While>();
    node->line = advance().line;                       // 'while'
    expect(TokenType::LParen, "'(' after 'while'");
    node->condition = parseExpression();
    expect(TokenType::RParen, "')' to close the condition");
    expect(TokenType::Newline, "end of line after while(...)");
    node->body = parseBlock();
    expectCloser("endwhile", "while", node->line);
    return node;
}

// A call on its own line: add(1, 2)  -> the result is printed at runtime.
StmtPtr Parser::parseExprStatement() {
    auto node = std::make_unique<ExprStmt>();
    node->line = peek().line;
    node->expr = parseCall();
    return node;
}

// ============================================================================
// EXPRESSIONS
// ============================================================================

// expression = call | VARIABLE [ "[" expression "]" ] | NUMBER | STRING
//            | BOOLEAN | FILE | FOLDER | list ;
ExprPtr Parser::parseExpression() {
    const Token& t = peek();
    switch (t.type) {
        case TokenType::Number: {
            auto n = std::make_unique<NumberLit>();
            n->line = t.line;
            n->value = std::stod(t.text);
            advance();
            return n;
        }
        case TokenType::String: {
            auto n = std::make_unique<StringLit>();
            n->line = t.line;
            n->value = t.text;
            advance();
            return n;
        }
        case TokenType::Boolean: {
            auto n = std::make_unique<BoolLit>();
            n->line = t.line;
            n->value = (t.text == "true");
            advance();
            return n;
        }
        case TokenType::File: {
            auto n = std::make_unique<FileLit>();
            n->line = t.line;
            n->text = t.text;
            advance();
            return n;
        }
        case TokenType::Folder: {
            auto n = std::make_unique<FolderLit>();
            n->line = t.line;
            n->text = t.text;
            advance();
            return n;
        }
        case TokenType::Variable:  return parseVarRef();
        case TokenType::LBracket:  return parseList();
        case TokenType::Function:  return parseCall();
        default: break;
    }
    fail("expected a value but found " + describe(t));
}

// VARIABLE [ "[" expression "]" ]     -x-   or   -x-[0]
ExprPtr Parser::parseVarRef() {
    auto node = std::make_unique<VarRef>();
    const Token& var = expect(TokenType::Variable, "a variable");
    node->line = var.line;
    node->name = var.text;
    if (match(TokenType::LBracket)) {
        node->index = parseExpression();
        expect(TokenType::RBracket, "']' to close the index");
    }
    return node;
}

// list = "[" [ expression { "," expression } ] "]" ;
ExprPtr Parser::parseList() {
    auto node = std::make_unique<ListLit>();
    node->line = advance().line;                       // '['
    if (!check(TokenType::RBracket)) {
        node->items.push_back(parseExpression());
        while (match(TokenType::Comma))
            node->items.push_back(parseExpression());
    }
    expect(TokenType::RBracket, "']' to close the list");
    return node;
}

// call = FUNCTION "(" [ expression { "," expression } ] ")" ;
// The "[ ... ]" makes the whole argument list optional; the "{ ... }" repeats
// "comma + expression" zero or more times. Hence add(), add(1), add(1,2,3).
// Because arguments are expressions, calls nest: add(1, mult(2, 3)).
ExprPtr Parser::parseCall() {
    auto node = std::make_unique<Call>();
    const Token& fn = expect(TokenType::Function, "a function name");
    node->line = fn.line;
    node->name = fn.text;
    expect(TokenType::LParen, "'(' after '" + fn.text + "'");
    if (!check(TokenType::RParen)) {
        node->args.push_back(parseExpression());
        while (match(TokenType::Comma))
            node->args.push_back(parseExpression());   // add(5,,10) fails here
    }
    expect(TokenType::RParen, "')' to close " + fn.text + "(");
    return node;
}
