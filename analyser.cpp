#include "Analyser.h"

// ============================================================================
// analyser.cpp  -  semantic checks before execution
//
// This stage verifies that the AST is not only syntactically valid but also
// makes sense in the language's type system. It tracks variable declarations,
// checks assignment compatibility, validates function calls, and ensures
// conditions and list indexes are used correctly.
//
// Design note:
//   - variables live in a single shared symbol table for the whole program
//   - every expression gets a static type before the code runs
//   - built-in calls are checked for arity and kind compatibility here
//   - runtime-only failures (missing files, divide-by-zero) are left for the
//     interpreter so the analyser stays focused on meaning and safety.
// ============================================================================

namespace {
constexpr size_t MANY = static_cast<size_t>(-1);     // "no upper limit" for argument counts

Type simple(Kind k) { return Type{k, Kind::Any}; }

// Does type `t` satisfy `wanted`?  Kind::Any (e.g. an element of a plain
// `list`) is accepted everywhere; the interpreter double-checks at runtime.
bool is(const Type& t, Kind wanted) { return t.kind == wanted || t.kind == Kind::Any; }

// File commands accept a file, a folder, or a string (strings convert to paths).
bool isPathLike(const Type& t) {
    return is(t, Kind::File) || is(t, Kind::Folder) || is(t, Kind::String);
}

bool oneOf(const std::string& n, std::initializer_list<const char*> names) {
    for (const char* s : names) if (n == s) return true;
    return false;
}
}  // namespace

void Analyser::fail(int line, const std::string& message) const {
    throw PunchError("semantic", line, message);
}

// ============================================================================
// ENTRY POINT
// ============================================================================
void Analyser::analyse(const Program& program) {
    symbols.clear();
    checkBlock(program.statements);
}

// ============================================================================
// STATEMENTS
// Blocks do not create a new scope: all variables share one global scope.
// ============================================================================
void Analyser::checkBlock(const Block& block) {
    for (const StmtPtr& s : block) checkStatement(*s);
}

void Analyser::checkStatement(const Stmt& s) {
    if (auto* d = dynamic_cast<const Declaration*>(&s)) { checkDeclaration(*d); return; }
    if (auto* c = dynamic_cast<const Change*>(&s))      { checkChange(*c); return; }

    if (auto* i = dynamic_cast<const If*>(&s)) {
        requireCondition(*i->condition, "if");
        checkBlock(i->thenBlock);
        checkBlock(i->elseBlock);
        return;
    }
    if (auto* l = dynamic_cast<const Loop*>(&s)) {
        Type t = typeOf(*l->count);
        if (!is(t, Kind::Number))
            fail(l->line, "loop count must be a number but is " + typeName(t));
        checkBlock(l->body);
        return;
    }
    if (auto* w = dynamic_cast<const While*>(&s)) {
        requireCondition(*w->condition, "while");
        checkBlock(w->body);
        return;
    }
    if (auto* e = dynamic_cast<const ExprStmt*>(&s)) {
        typeOf(*e->expr);       // checks the call is valid; result type is ignored
        return;
    }
}

// -name-type = value
void Analyser::checkDeclaration(const Declaration& d) {
    if (symbols.count(d.name))
        fail(d.line, "'-" + d.name + "-' is already declared (use change() to update it)");

    // Work out the value's type BEFORE adding the name, so
    // `-x-number = add(-x-, 1)` is correctly reported as use-before-declare.
    Type actual = typeOf(*d.value);
    if (actual.kind == Kind::Void) fail(d.line, "the value has no result to store");
    if (!assignable(d.type, *d.value, actual))
        fail(d.line, mismatch(d.type, *d.value, actual) + " ('-" + d.name +
                     "-' is declared " + typeName(d.type) + ")");
    symbols[d.name] = d.type;
}

// change(-name-, value)   or   change(-name-[i], value)
void Analyser::checkChange(const Change& c) {
    const VarRef& t = *c.target;
    auto it = symbols.find(t.name);
    if (it == symbols.end())
        fail(c.line, "'-" + t.name + "-' is not declared");

    Type target = it->second;
    if (t.index) {
        // change(-list-[i], v): the target is ONE ELEMENT of the list
        if (target.kind != Kind::List) fail(c.line, "'-" + t.name + "-' is not a list, so it cannot be indexed");
        if (!is(typeOf(*t.index), Kind::Number)) fail(c.line, "a list index must be a number");
        target = simple(target.elem);
    } else if (target.kind == Kind::File) {
        // The language rule: files are renamed or deleted, never reassigned.
        fail(c.line, "file variable '-" + t.name + "-' cannot be changed (use rename() or delete())");
    }

    Type actual = typeOf(*c.value);
    if (actual.kind == Kind::Void) fail(c.line, "the new value has no result");
    if (target.kind != Kind::Any && !assignable(target, *c.value, actual))
        fail(c.line, mismatch(target, *c.value, actual) + " ('-" + t.name +
                     "-' holds " + typeName(target) + ")");
}

// The wording for a type mismatch. For a list literal going into list_X it
// names the first item of the wrong type, which is far more useful than
// just saying "list".
std::string Analyser::mismatch(const Type& declared, const Expr& value, const Type& actual) {
    if (declared.kind == Kind::List && actual.kind == Kind::List && declared.elem != Kind::Any) {
        if (auto* lit = dynamic_cast<const ListLit*>(&value)) {
            for (size_t i = 0; i < lit->items.size(); i++) {
                Type et = typeOf(*lit->items[i]);
                if (et.kind != declared.elem && et.kind != Kind::Any)
                    return "list item " + std::to_string(i) + " is a " + typeName(et) +
                           " but the list must hold " + kindName(declared.elem) + "s";
            }
        }
    }
    return "cannot store " + typeName(actual);
}

// Is `value` (of type `actual`) allowed where `declared` is expected?
bool Analyser::assignable(const Type& declared, const Expr& value, const Type& actual) {
    if (actual.kind == Kind::Any) return true;            // unknown until runtime
    if (declared.kind != actual.kind) return false;
    if (declared.kind != Kind::List) return true;
    if (declared.elem == Kind::Any) return true;          // plain `list` takes any list

    // list_X: a list LITERAL is checked element by element ...
    if (auto* lit = dynamic_cast<const ListLit*>(&value)) {
        for (const ExprPtr& item : lit->items) {
            Type et = typeOf(*item);
            if (et.kind != declared.elem && et.kind != Kind::Any) return false;
        }
        return true;
    }
    // ... anything else (a variable, in_file(...)) must carry the same element type.
    return actual.elem == declared.elem;
}

void Analyser::requireCondition(const Expr& e, const std::string& what) {
    Type t = typeOf(e);
    if (!is(t, Kind::Boolean))
        fail(e.line, what + " condition must be true/false (e.g. lt(), eq(), and()) but is " + typeName(t));
}

// ============================================================================
// EXPRESSIONS: compute each expression's type
// ============================================================================
Type Analyser::typeOf(const Expr& e) {
    if (dynamic_cast<const NumberLit*>(&e)) return simple(Kind::Number);
    if (dynamic_cast<const StringLit*>(&e)) return simple(Kind::String);
    if (dynamic_cast<const BoolLit*>(&e))   return simple(Kind::Boolean);
    if (dynamic_cast<const FileLit*>(&e))   return simple(Kind::File);
    if (dynamic_cast<const FolderLit*>(&e)) return simple(Kind::Folder);
    if (auto* l = dynamic_cast<const ListLit*>(&e)) return typeOfList(*l);
    if (auto* v = dynamic_cast<const VarRef*>(&e))  return typeOfVarRef(*v);
    if (auto* c = dynamic_cast<const Call*>(&e))    return typeOfCall(*c);
    fail(e.line, "unknown expression");
}

// -x-  or  -x-[i]
Type Analyser::typeOfVarRef(const VarRef& v) {
    auto it = symbols.find(v.name);
    if (it == symbols.end()) fail(v.line, "'-" + v.name + "-' is used before it is declared");

    if (!v.index) return it->second;

    if (it->second.kind != Kind::List) fail(v.line, "'-" + v.name + "-' is not a list, so it cannot be indexed");
    if (!is(typeOf(*v.index), Kind::Number)) fail(v.line, "a list index must be a number");
    return simple(it->second.elem);            // an element has the list's element type
}

// [a, b, c]: if every item has the same kind the list is list_<kind>,
// otherwise it is a mixed list (element kind Any).
Type Analyser::typeOfList(const ListLit& l) {
    Kind common = Kind::Any;
    bool first = true, mixed = false;
    for (const ExprPtr& item : l.items) {
        Type t = typeOf(*item);
        if (t.kind == Kind::Void) fail(l.line, "a list item has no value");
        if (t.kind == Kind::List) fail(l.line, "nested lists are not supported");
        if (first) { common = t.kind; first = false; }
        else if (t.kind != common) mixed = true;
    }
    if (mixed || first) common = Kind::Any;    // mixed or empty
    return Type{Kind::List, common};
}

// ============================================================================
// FUNCTION CALLS: the signature table
// ============================================================================
void Analyser::needArgs(const Call& c, size_t min, size_t max) {
    size_t n = c.args.size();
    if (n >= min && n <= max) return;
    std::string expected = (min == max)   ? "exactly " + std::to_string(min)
                         : (max == MANY)  ? "at least " + std::to_string(min)
                                          : std::to_string(min) + " to " + std::to_string(max);
    fail(c.line, c.name + "() takes " + expected + " argument(s) but got " + std::to_string(n));
}

void Analyser::requireKind(const Call& c, const std::vector<Type>& a, size_t i, Kind wanted) {
    if (!is(a[i], wanted))
        fail(c.line, "argument " + std::to_string(i + 1) + " of " + c.name + "() must be a " +
                     kindName(wanted) + " but is " + typeName(a[i]));
}

void Analyser::requirePathLike(const Call& c, const std::vector<Type>& a, size_t i) {
    if (!isPathLike(a[i]))
        fail(c.line, "argument " + std::to_string(i + 1) + " of " + c.name +
                     "() must be a file, folder or name but is " + typeName(a[i]));
}

Type Analyser::typeOfCall(const Call& c) {
    // Type every argument first (this also checks nested calls).
    std::vector<Type> a;
    for (size_t i = 0; i < c.args.size(); i++) {
        Type t = typeOf(*c.args[i]);
        if (t.kind == Kind::Void)
            fail(c.line, "argument " + std::to_string(i + 1) + " of " + c.name + "() has no value");
        a.push_back(t);
    }
    const std::string& n = c.name;

    // arithmetic: one or more numbers -> number
    if (oneOf(n, {"add", "sub", "mult", "div"})) {
        needArgs(c, 1, MANY);
        for (size_t i = 0; i < a.size(); i++) requireKind(c, a, i, Kind::Number);
        return simple(Kind::Number);
    }
    // ordering: two numbers -> boolean
    if (oneOf(n, {"lt", "gt"})) {
        needArgs(c, 2, 2);
        requireKind(c, a, 0, Kind::Number);
        requireKind(c, a, 1, Kind::Number);
        return simple(Kind::Boolean);
    }
    // equality: two values of the same kind -> boolean
    if (oneOf(n, {"eq", "neq"})) {
        needArgs(c, 2, 2);
        if (a[0].kind != a[1].kind && a[0].kind != Kind::Any && a[1].kind != Kind::Any)
            fail(c.line, n + "() compares a " + typeName(a[0]) + " with a " + typeName(a[1]));
        return simple(Kind::Boolean);
    }
    // logic: booleans -> boolean
    if (oneOf(n, {"and", "or"})) {
        needArgs(c, 2, MANY);
        for (size_t i = 0; i < a.size(); i++) requireKind(c, a, i, Kind::Boolean);
        return simple(Kind::Boolean);
    }
    if (n == "not") {
        needArgs(c, 1, 1);
        requireKind(c, a, 0, Kind::Boolean);
        return simple(Kind::Boolean);
    }

    // file commands
    if (n == "rename") {
        needArgs(c, 2, 2);
        requirePathLike(c, a, 0);
        requireKind(c, a, 1, Kind::String);
        return simple(Kind::Void);
    }
    if (oneOf(n, {"delete", "wipe", "duplicate"})) {
        needArgs(c, 1, 1);
        requirePathLike(c, a, 0);
        return simple(Kind::Void);
    }
    if (n == "size") {
        needArgs(c, 1, 1);
        requirePathLike(c, a, 0);
        return simple(Kind::Number);
    }

    // input / output
    if (n == "in") {
        needArgs(c, 0, 0);
        return simple(Kind::String);
    }
    if (n == "in_file") {
        needArgs(c, 1, 1);
        requirePathLike(c, a, 0);
        return Type{Kind::List, Kind::String};
    }
    if (n == "out") {
        needArgs(c, 0, MANY);
        return simple(Kind::Void);
    }
    if (oneOf(n, {"out_file", "out_append"})) {
        needArgs(c, 1, MANY);
        requirePathLike(c, a, 0);
        return simple(Kind::Void);
    }

    fail(c.line, "unknown function '" + n + "'");
}
