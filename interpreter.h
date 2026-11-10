#pragma once
// ============================================================================
// interpreter.h  -  the runtime engine for Punch programs
//
// The interpreter is the final stage in the pipeline. It walks the verified AST
// and carries out the program's instructions directly in C++. There is no
// bytecode or JIT layer; statements are executed recursively and expressions are
// evaluated to runtime values as needed.
//
// The runtime environment is a simple map from variable name to current Value.
// Because the analyser enforces a single global scope, the interpreter can look
// up variables by name and execute built-in language operations such as math,
// comparisons, I/O, and filesystem commands.
// ============================================================================
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include "Ast.h"
#include "Errors.h"
#include "Value.h"

// The interpreter is the runtime stage. After the parser builds an AST and the
// analyser confirms the program is valid, this class executes the AST directly.
// The environment is intentionally tiny: a map from variable name to Value.
class Interpreter {
public:
    void run(const Program& program);

private:
    std::unordered_map<std::string, Value> env;       // variable memory

    // ---- statements -------------------------------------------------------
    void execBlock(const Block& block);
    void exec(const Stmt& s);
    void execDeclaration(const Declaration& d);
    void execChange(const Change& c);
    void execIf(const If& i);
    void execLoop(const Loop& l);
    void execWhile(const While& w);
    void execExprStmt(const ExprStmt& e);

    // ---- expressions ------------------------------------------------------
    Value eval(const Expr& e);
    Value evalVarRef(const VarRef& v);
    Value evalList(const ListLit& l);
    Value evalCall(const Call& c);

    // ---- built-in function groups (each returns the call's result) ---------
    Value callMath(const std::string& name, const std::vector<Value>& args, int line);
    Value callCompare(const std::string& name, const std::vector<Value>& args, int line);
    Value callLogic(const std::string& name, const std::vector<Value>& args, int line);
    Value callFile(const Call& call, const std::vector<Value>& args);
    Value callIO(const std::string& name, const std::vector<Value>& args, int line);

    // ---- file helpers -----------------------------------------------------
    // The headline feature: current folder first, then DOWN through sub-folders.
    std::filesystem::path resolve(const std::string& name, int line, bool mustExist);
};
