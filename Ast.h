#pragma once
// ============================================================================
// ast.h  -  the Abstract Syntax Tree (AST)
//
// After tokenization and parsing, the program is represented as a tree of
// objects rather than a flat token stream. The AST is the program's structured
// meaning: it tells the analyser and interpreter what actions to validate and
// execute.
//
// Each node in the tree corresponds to one grammar rule from the language:
//
//   Expressions (produce a value)      Statements (do something)
//   -----------------------------      -------------------------
//   NumberLit  42                      Declaration  -x-number = 42
//   StringLit  "hi"                    Change       change(-x-, 5)
//   BoolLit    true                    If           if(...) ... endif
//   FileLit    notes.txt               Loop         loop(3) ... endloop
//   FolderLit  docs/                   While        while(...) ... endwhile
//   ListLit    [1, 2, 3]               ExprStmt     add(1, 2)
//   VarRef     -x-  or  -x-[0]
//   Call       add(1, mult(2, 3))
//
// Every node records its source line so the compiler can report problems at
// the exact location in the user's script. Ownership is handled with
// std::unique_ptr so parent nodes can contain child nodes without manual memory
// management.
// ============================================================================
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>
#include "Value.h"

// Common base. The virtual destructor lets us delete a derived node through a
// base pointer, and enables dynamic_cast to find out which node we have.
struct Node {
    int line = 0;
    virtual ~Node() = default;
};
struct Expr : Node {};
struct Stmt : Node {};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using Block   = std::vector<StmtPtr>;      // a run of statements, in order

// The AST is the compiler's internal model of the program. It is tree-shaped so
// nested expressions such as add(1, mult(2, 3)) can be evaluated in a predictable
// order, while control-flow blocks remain easy to validate and execute.

// ----- expressions ---------------------------------------------------------
struct NumberLit : Expr { double value = 0; };
struct StringLit : Expr { std::string value; };
struct BoolLit   : Expr { bool value = false; };
struct FileLit   : Expr { std::string text; };
struct FolderLit : Expr { std::string text; };
struct ListLit   : Expr { std::vector<ExprPtr> items; };

// A variable use. `index` is only set for  -list-[expr].
struct VarRef : Expr {
    std::string name;
    ExprPtr index;
};

// A built-in function call: name(arg, arg, ...).
struct Call : Expr {
    std::string name;
    std::vector<ExprPtr> args;
};

// ----- statements ----------------------------------------------------------
struct Declaration : Stmt {        // -name-type = value
    std::string name;
    Type type;
    ExprPtr value;
};
struct Change : Stmt {             // change(-name-, value)  or  change(-list-[i], value)
    std::unique_ptr<VarRef> target;
    ExprPtr value;
};
struct If : Stmt {
    ExprPtr condition;
    Block thenBlock;
    Block elseBlock;               // empty if there is no else
};
struct Loop : Stmt {               // loop(n) ... endloop
    ExprPtr count;
    Block body;
};
struct While : Stmt {              // while(cond) ... endwhile
    ExprPtr condition;
    Block body;
};
struct ExprStmt : Stmt {           // a call on its own line; its result is printed
    ExprPtr expr;
};

struct Program {
    Block statements;
};

// Debug helpers (used by  ./punch --ast file.punch ).
std::string exprToString(Expr& e);              // back to source-like text
void printProgram(Program& program, std::ostream& os);
