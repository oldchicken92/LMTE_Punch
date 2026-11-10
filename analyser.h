#pragma once
// ============================================================================
// analyser.h  -  semantic validation pass
//
// The parser is responsible for structure; this stage is responsible for
// meaning. It walks the AST with a symbol table and rejects programs that use
// variables incorrectly, assign incompatible values, call built-ins with the
// wrong signature, or apply operations to the wrong kinds of data.
//
// This pass acts like a static checker. It proves as much as possible before a
// script starts running, while leaving runtime-only issues such as missing files
// or division by zero to the interpreter.
// ============================================================================
#include <string>
#include <unordered_map>
#include <vector>
#include "Ast.h"
#include "Errors.h"
#include "Value.h"

// Static analysis happens before execution. This class validates the AST using a
// symbol table and type rules so the runtime can assume the program is meaningfully
// well-formed and only fail on genuinely dynamic issues.
class Analyser {
public:
    void analyse(const Program& program);       // throws PunchError("semantic", ...)

private:
    std::unordered_map<std::string, Type> symbols;   // the symbol table

    // ---- statements -------------------------------------------------------
    void checkBlock(const Block& block);
    void checkStatement(const Stmt& s);
    void checkDeclaration(const Declaration& d);
    void checkChange(const Change& c);

    // ---- expressions: each returns the TYPE of the expression ----------------
    Type typeOf(const Expr& e);
    Type typeOfVarRef(const VarRef& v);
    Type typeOfList(const ListLit& l);
    Type typeOfCall(const Call& c);

    // ---- helpers ----------------------------------------------------------
    bool assignable(const Type& declared, const Expr& value, const Type& actual);
    std::string mismatch(const Type& declared, const Expr& value, const Type& actual);
    void needArgs(const Call& c, size_t min, size_t max);
    void requireKind(const Call& c, const std::vector<Type>& a, size_t i, Kind wanted);
    void requirePathLike(const Call& c, const std::vector<Type>& a, size_t i);
    void requireCondition(const Expr& e, const std::string& what);
    [[noreturn]] void fail(int line, const std::string& message) const;
};
