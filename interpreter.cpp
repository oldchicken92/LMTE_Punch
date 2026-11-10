#include "Interpreter.h"
#include <cmath>
#include <fstream>
#include <iostream>

// ============================================================================
// interpreter.cpp  -  execute a validated Punch program
//
// This file is the runtime core. It evaluates expressions, stores variable
// state, executes control-flow statements, and implements the built-in language
// functions (math, comparison, logic, filesystem commands, and I/O).
//
// The runtime is intentionally simple:
//   - env maps names to the current Value held by the variable
//   - exec*() functions perform the actual statement work
//   - eval*() functions recursively compute expression results
//   - built-in functions are grouped by purpose (math, compare, logic, file, I/O)
//
// Because the analyser already checks most invalid code, the interpreter mostly
// focuses on executing programs that are known to be structurally and semantically
// valid.
// ============================================================================

namespace fs = std::filesystem;

// ============================================================================
// SMALL HELPERS (private to this file)
// ============================================================================
namespace {

// Every runtime problem goes through here so it carries the line number.
[[noreturn]] void runtimeError(int line, const std::string& message) {
    throw PunchError("runtime", line, message);
}

// Pull a typed value out of a Value. The analyser already checked types, so
// these only fail for things it could not know (e.g. an element of a plain list).
double asNumber(const Value& v, int line) {
    if (auto* d = std::get_if<double>(&v.data)) return *d;
    runtimeError(line, "expected a number but got '" + toText(v) + "'");
}
bool asBool(const Value& v, int line) {
    if (auto* b = std::get_if<bool>(&v.data)) return *b;
    runtimeError(line, "expected true/false but got '" + toText(v) + "'");
}
// File commands accept a string, a file or a folder: all give a name/path.
std::string asPathText(const Value& v, int line) {
    if (auto* s = std::get_if<std::string>(&v.data)) return *s;
    if (auto* p = std::get_if<Path>(&v.data)) return p->p.string();
    runtimeError(line, "expected a file, folder or name but got '" + toText(v) + "'");
}

// Concatenate arguments (from index `from`) as text, with NO separator.
std::string joinText(const std::vector<Value>& args, size_t from) {
    std::string out;
    for (size_t i = from; i < args.size(); i++) out += toText(args[i]);
    return out;
}

// Validate a list index: must be 0 .. size-1.
size_t checkedIndex(double index, size_t size, int line) {
    if (index < 0 || index >= static_cast<double>(size))
        runtimeError(line, "index " + toText(Value(index)) + " is out of range (list has " +
                           std::to_string(size) + " items)");
    return static_cast<size_t>(index);        // fractions round down
}

bool oneOf(const std::string& n, std::initializer_list<const char*> names) {
    for (const char* s : names) if (n == s) return true;
    return false;
}
}  // namespace

// ============================================================================
// ENTRY POINT
// ============================================================================
void Interpreter::run(const Program& program) {
    env.clear();
    execBlock(program.statements);
}

// ============================================================================
// STATEMENTS  (execute: do something)
// ============================================================================
void Interpreter::execBlock(const Block& block) {
    for (const StmtPtr& s : block) exec(*s);
}

// Find out which statement this is, then run the matching function.
void Interpreter::exec(const Stmt& s) {
    if (auto* d = dynamic_cast<const Declaration*>(&s)) return execDeclaration(*d);
    if (auto* c = dynamic_cast<const Change*>(&s))      return execChange(*c);
    if (auto* i = dynamic_cast<const If*>(&s))          return execIf(*i);
    if (auto* l = dynamic_cast<const Loop*>(&s))        return execLoop(*l);
    if (auto* w = dynamic_cast<const While*>(&s))       return execWhile(*w);
    if (auto* e = dynamic_cast<const ExprStmt*>(&s))    return execExprStmt(*e);
}

// -name-type = value   ->  store the value under the name.
// (Declared inside a loop body, this simply re-stores each pass.)
void Interpreter::execDeclaration(const Declaration& d) {
    env[d.name] = eval(*d.value);
}

// change(-name-, value)  or  change(-list-[i], value)
void Interpreter::execChange(const Change& c) {
    Value v = eval(*c.value);
    const VarRef& target = *c.target;

    auto it = env.find(target.name);
    if (it == env.end()) runtimeError(c.line, "'-" + target.name + "-' has no value yet");

    if (target.index) {
        auto* list = std::get_if<List>(&it->second.data);
        if (!list) runtimeError(c.line, "'-" + target.name + "-' is not a list");
        size_t i = checkedIndex(asNumber(eval(*target.index), c.line), list->size(), c.line);
        (*list)[i] = std::move(v);
    } else {
        it->second = std::move(v);
    }
}

void Interpreter::execIf(const If& i) {
    if (asBool(eval(*i.condition), i.line)) execBlock(i.thenBlock);
    else                                    execBlock(i.elseBlock);
}

// loop(n): n must be >= 0; a decimal rounds down (loop(2.9) runs twice).
void Interpreter::execLoop(const Loop& l) {
    double n = asNumber(eval(*l.count), l.line);
    if (n < 0) runtimeError(l.line, "loop count must not be negative (got " + toText(Value(n)) + ")");
    long times = static_cast<long>(std::floor(n));
    for (long k = 0; k < times; k++) execBlock(l.body);
}

// while: the condition is re-checked before every pass.
// (An endless loop is the programmer's responsibility.)
void Interpreter::execWhile(const While& w) {
    while (asBool(eval(*w.condition), w.line)) execBlock(w.body);
}

// A call on its own line. If it produced a value, print it (so `add(1, 2)`
// shows 3). Commands with no result (out, delete ...) print nothing extra.
void Interpreter::execExprStmt(const ExprStmt& e) {
    Value v = eval(*e.expr);
    if (!std::holds_alternative<std::monostate>(v.data))
        std::cout << toText(v) << "\n";
}

// ============================================================================
// EXPRESSIONS  (evaluate: compute a Value)
// ============================================================================
Value Interpreter::eval(const Expr& e) {
    if (auto* n = dynamic_cast<const NumberLit*>(&e)) return Value(n->value);
    if (auto* s = dynamic_cast<const StringLit*>(&e)) return Value(s->value);
    if (auto* b = dynamic_cast<const BoolLit*>(&e))   return Value(b->value);
    if (auto* f = dynamic_cast<const FileLit*>(&e))   return Value(Path{fs::path(f->text), false});
    if (auto* f = dynamic_cast<const FolderLit*>(&e)) return Value(Path{fs::path(f->text), true});
    if (auto* l = dynamic_cast<const ListLit*>(&e))   return evalList(*l);
    if (auto* v = dynamic_cast<const VarRef*>(&e))    return evalVarRef(*v);
    if (auto* c = dynamic_cast<const Call*>(&e))      return evalCall(*c);
    runtimeError(e.line, "cannot evaluate this expression");
}

// -x-  or  -x-[i]
Value Interpreter::evalVarRef(const VarRef& v) {
    auto it = env.find(v.name);
    if (it == env.end()) runtimeError(v.line, "'-" + v.name + "-' has no value yet");
    if (!v.index) return it->second;

    auto* list = std::get_if<List>(&it->second.data);
    if (!list) runtimeError(v.line, "'-" + v.name + "-' is not a list");
    size_t i = checkedIndex(asNumber(eval(*v.index), v.line), list->size(), v.line);
    return (*list)[i];
}

// [a, b, c]: evaluate each item in order.
Value Interpreter::evalList(const ListLit& l) {
    List items;
    for (const ExprPtr& item : l.items) items.push_back(eval(*item));
    return Value(std::move(items));
}

// Evaluate all arguments FIRST (inner calls run before outer ones, which is
// why nesting defines evaluation order), then dispatch by function name.
Value Interpreter::evalCall(const Call& c) {
    std::vector<Value> args;
    for (const ExprPtr& a : c.args) args.push_back(eval(*a));
    const std::string& n = c.name;

    if (oneOf(n, {"add", "sub", "mult", "div"}))        return callMath(n, args, c.line);
    if (oneOf(n, {"lt", "gt", "eq", "neq"}))            return callCompare(n, args, c.line);
    if (oneOf(n, {"and", "or", "not"}))                 return callLogic(n, args, c.line);
    if (oneOf(n, {"rename", "delete", "wipe", "size", "duplicate"})) return callFile(c, args);
    if (oneOf(n, {"in", "in_file", "out", "out_file", "out_append"})) return callIO(n, args, c.line);
    runtimeError(c.line, "unknown function '" + n + "'");
}

// ============================================================================
// BUILT-IN FUNCTIONS: maths, comparison, logic
// ============================================================================

// Arithmetic folds left to right:  sub(10, 3, 2) = (10 - 3) - 2 = 5
Value Interpreter::callMath(const std::string& name, const std::vector<Value>& args, int line) {
    double acc = asNumber(args[0], line);
    for (size_t i = 1; i < args.size(); i++) {
        double x = asNumber(args[i], line);
        if      (name == "add")  acc += x;
        else if (name == "sub")  acc -= x;
        else if (name == "mult") acc *= x;
        else {                                       // div
            if (x == 0) runtimeError(line, "division by zero");
            acc /= x;
        }
    }
    // div(1, 0) with a single argument never divides, so nothing to check there.
    return Value(acc);
}

Value Interpreter::callCompare(const std::string& name, const std::vector<Value>& args, int line) {
    if (name == "eq")  return Value(valuesEqual(args[0], args[1]));
    if (name == "neq") return Value(!valuesEqual(args[0], args[1]));
    double a = asNumber(args[0], line), b = asNumber(args[1], line);
    return Value(name == "lt" ? a < b : a > b);
}

Value Interpreter::callLogic(const std::string& name, const std::vector<Value>& args, int line) {
    if (name == "not") return Value(!asBool(args[0], line));
    bool isAnd = (name == "and");
    bool result = isAnd;                             // and starts true, or starts false
    for (const Value& v : args) {
        bool b = asBool(v, line);
        result = isAnd ? (result && b) : (result || b);
    }
    return Value(result);
}

// ============================================================================
// FILE COMMANDS and the DOWNWARD SEARCH RULE
// ============================================================================

// resolve(name) finds the real path a name refers to:
//   1. absolute paths and ".." are rejected, so Punch can never climb upwards
//   2. a name containing '/' (docs/a.txt) is used exactly as written
//   3. a plain name is looked for in the CURRENT folder first
//   4. otherwise every sub-folder is searched (symlinks are not followed)
//        0 matches -> "not found"   (or the name itself, if mustExist is false)
//        1 match   -> use it
//        2+ matches-> ambiguity error listing them; nothing is touched
fs::path Interpreter::resolve(const std::string& name, int line, bool mustExist) {
    fs::path wanted(name);

    if (wanted.is_absolute())
        runtimeError(line, "'" + name + "': absolute paths are not allowed (Punch only searches downwards)");
    for (const fs::path& part : wanted)
        if (part == "..")
            runtimeError(line, "'" + name + "': '..' is not allowed (Punch only searches downwards)");

    // "docs/" has no filename part; drop the trailing slash.
    if (!wanted.has_filename()) wanted = wanted.parent_path();

    // Names with a folder part are taken literally.
    if (wanted.has_parent_path()) {
        if (mustExist && !fs::exists(wanted)) runtimeError(line, "'" + name + "' was not found");
        return wanted;
    }

    // Current folder wins over anything deeper.
    if (fs::exists(wanted)) return wanted;

    // Search sub-folders.
    std::vector<fs::path> matches;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(".", fs::directory_options::skip_permission_denied, ec), end;
         it != end; it.increment(ec)) {
        if (it->is_symlink(ec)) continue;                      // never follow links
        if (it->path().filename() == wanted) matches.push_back(it->path().lexically_normal());
    }

    if (matches.empty()) {
        if (mustExist) runtimeError(line, "'" + name + "' was not found here or in any sub-folder");
        return wanted;                                         // e.g. out_file creates it here
    }
    if (matches.size() > 1) {
        std::string list;
        for (const fs::path& m : matches) list += (list.empty() ? "" : ", ") + m.string();
        runtimeError(line, "'" + name + "' is ambiguous, found " + std::to_string(matches.size()) +
                           " matches: " + list);
    }
    return matches[0];
}

// rename, delete, wipe, size, duplicate.
// std::filesystem throws on OS-level failures; we turn those into Punch errors.
Value Interpreter::callFile(const Call& call, const std::vector<Value>& args) {
    const std::string& n = call.name;
    int line = call.line;

    try {
        fs::path target = resolve(asPathText(args[0], line), line, true);

        // rename(-file-, "new"): rename on disk, keep contents.
        if (n == "rename") {
            std::string newName = asPathText(args[1], line);
            if (newName.find('/') != std::string::npos)
                runtimeError(line, "the new name must not contain '/'");
            fs::path newPath = target.parent_path() / newName;
            if (fs::exists(newPath)) runtimeError(line, "'" + newName + "' already exists");
            fs::rename(target, newPath);

            // Keep the variable pointing at the renamed file.
            if (auto* ref = dynamic_cast<const VarRef*>(call.args[0].get())) {
                auto it = env.find(ref->name);
                if (!ref->index && it != env.end())
                    if (auto* p = std::get_if<Path>(&it->second.data)) p->p = newName;
            }
            return Value();
        }

        // delete: a file, or an EMPTY folder (never a folder with contents).
        if (n == "delete") {
            if (fs::is_directory(target) && !fs::is_empty(target))
                runtimeError(line, "folder '" + target.string() + "' is not empty (use wipe() first)");
            fs::remove(target);
            return Value();
        }

        // wipe: remove everything INSIDE a folder, keep the folder itself.
        if (n == "wipe") {
            if (!fs::is_directory(target))
                runtimeError(line, "'" + target.string() + "' is not a folder");
            for (const auto& entry : fs::directory_iterator(target))
                fs::remove_all(entry.path());
            return Value();
        }

        // size: bytes in a file.
        if (n == "size") {
            if (fs::is_directory(target))
                runtimeError(line, "'" + target.string() + "' is a folder; size() needs a file");
            return Value(static_cast<double>(fs::file_size(target)));
        }

        // duplicate: copy to <name>_copy<.ext> next to the original.
        if (!fs::is_regular_file(target))
            runtimeError(line, "'" + target.string() + "' is not a file, so it cannot be duplicated");
        fs::path copy = target.parent_path() /
                        (target.stem().string() + "_copy" + target.extension().string());
        if (fs::exists(copy)) runtimeError(line, "'" + copy.string() + "' already exists");
        fs::copy_file(target, copy);
        return Value();

    } catch (const fs::filesystem_error& e) {
        runtimeError(line, n + "() failed: " + e.code().message());
    }
}

// ============================================================================
// INPUT / OUTPUT
// ============================================================================
Value Interpreter::callIO(const std::string& name, const std::vector<Value>& args, int line) {
    // out(...): join the arguments (no separator) and end with a newline.
    if (name == "out") {
        std::cout << joinText(args, 0) << "\n";
        return Value();
    }

    // in(): read one line from the keyboard.
    if (name == "in") {
        std::string input;
        std::getline(std::cin, input);
        return Value(input);
    }

    try {
        // in_file(file): the file's lines as a list of strings.
        if (name == "in_file") {
            fs::path p = resolve(asPathText(args[0], line), line, true);
            std::ifstream in(p);
            if (!in) runtimeError(line, "cannot open '" + p.string() + "'");
            List lines;
            std::string text;
            while (std::getline(in, text)) lines.push_back(Value(text));
            return Value(std::move(lines));
        }

        // out_file / out_append: the file need not exist yet (resolve with
        // mustExist = false). out_file overwrites, out_append adds to the end.
        fs::path p = resolve(asPathText(args[0], line), line, false);
        if (fs::is_directory(p)) runtimeError(line, "'" + p.string() + "' is a folder");
        std::ofstream out(p, name == "out_file" ? std::ios::trunc : std::ios::app);
        if (!out) runtimeError(line, "cannot write to '" + p.string() + "'");
        out << joinText(args, 1) << "\n";
        return Value();

    } catch (const fs::filesystem_error& e) {
        runtimeError(line, name + "() failed: " + e.code().message());
    }
}
