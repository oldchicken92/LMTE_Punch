/*
Implements a recursive descent parser that turns tokens into the AST. 
Each grammar rule has its own function, and each function calls the 
rules it contains, so parseCall calls parseExpression, which can call 
parseCall again. One token of lookahead is enough because every statement 
starts with a distinctive token. parseBlock() reads statements until it 
reaches else or an end... keyword, and the opener then checks that it is the 
right closer, which also allows nesting. It reports syntax errors: a missing 
closer, a wrong closer, a stray else, two statements on one line, a missing 
argument, a missing bracket, and a variable declared without a type.
*/

#include "parser.h"
Parser::Parser(std::vector<Token> t) : tokens(std::move(t)) {}
