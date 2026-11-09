/*
Defines the Abstract Syntax Tree, one struct per grammar rule. Expressions are the 
literals, VarRef (a variable use) and Call. Statements are Declaration, Change, 
If, Loop, While and ExprStmt. Every node stores its line number, and the tree owns 
its children through unique_ptr. The parser builds this tree, and the analyser and 
interpreter both walk it.
*/

#pragma once
#include <memory>
#include <string>
#include <vector>

struct Expr { int line = 0; virtual ~Expr() = default; };
struct Stmt { int line = 0; virtual ~Stmt() = default; };

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Program { std::vector<StmtPtr> statements; };
