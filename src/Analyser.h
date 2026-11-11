/*
Analyser header file used for validation passemantic pass 

Ensures thast the parsed program makes sense before it executes
*/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Ast.h"
#include "Errors.h"
#include "Value.h"

// This validates the ast
class Analyser {
public:
    void analyse(const Program& program); // for punch error

private:
    std::unordered_map<std::string, Type> symbols; // declared variables and their types

    // statements
    void checkBlock(const Block& block); // checks statement block
    void checkStatement(const Stmt& s);
    void checkDeclaration(const Declaration& d);
    void checkChange(const Change& c);

    // expressions
    Type typeOf(const Expr& e);   // return type of expression
    Type typeOfVarRef(const VarRef& v);  // varaible read of indexed list element
    Type typeOfList(const ListLit& l);  // list literal type calc
    Type typeOfCall(const Call& c);  // function validation

    // helper functions
    bool assignable(const Type& declared, const Expr& value, const Type& actual);
    std::string mismatch(const Type& declared, const Expr& value, const Type& actual);
    void needArgs(const Call& c, size_t min, size_t max);
    void requireKind(const Call& c, const std::vector<Type>& a, size_t i, Kind wanted);
    void requirePathLike(const Call& c, const std::vector<Type>& a, size_t i);
    void requireCondition(const Expr& e, const std::string& what);
    [[noreturn]] void fail(int line, const std::string& message) const;
};
