#pragma once
// ============================================================================
// value.h  -  static type information and runtime value storage
//
// Punch separates "what the compiler knows" from "what the program is
// currently holding". The Kind/Type layer is used by the analyser to check
// declarations and function calls before execution, while the Value layer stores
// actual data inside variables at runtime.
//
// This split is important because the language supports both obvious literals
// like 42 and file/folder values that are only valid once the program is
// running in a real filesystem.
// ============================================================================
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

// ---------------------------------------------------------------------------
// Static types
// ---------------------------------------------------------------------------

// Every kind of thing an expression can be.
//   Void = "produces nothing" (out, delete, rename ...)
//   Any  = "unknown / could be anything" (elements of a plain `list`)
//
// This is the static type table used by the analyser. It tells the compiler what
// kind of value an expression should produce before a script runs.
enum class Kind { Number, String, Boolean, File, Folder, List, Void, Any };

// A full type. For lists, `elem` says what the list holds
// (Any means a plain `list` that may hold mixed things).
struct Type {
    Kind kind = Kind::Any;
    Kind elem = Kind::Any;
};
inline bool operator==(const Type& a, const Type& b) {
    return a.kind == b.kind && a.elem == b.elem;
}

std::string kindName(Kind k);                 // Kind::Number -> "number"
std::string typeName(const Type& t);          // list of strings -> "list_string"
// Turns the word written in source ("number", "list_string") into a Type.
// Returns false if the word is not a valid type.
bool parseTypeName(const std::string& text, Type& out);

// ---------------------------------------------------------------------------
// Runtime values
// ---------------------------------------------------------------------------
struct Value;
using List = std::vector<Value>;    // lists hold Values, so they can hold anything

// A file or folder. We store only the path; whether it exists on disk is
// checked at runtime by the command that uses it.
struct Path {
    std::filesystem::path p;
    bool isFolder = false;
};

// One C++ type that can hold any Punch value. std::variant holds exactly ONE
// of the alternatives at a time. monostate means "no value" (Void).
struct Value {
    std::variant<std::monostate, double, bool, std::string, Path, List> data;

    Value() = default;                                   // a "no value" Value
    template <class T>
    explicit Value(T v) : data(std::move(v)) {}          // Value(3.0), Value(std::string("hi"))
};

// Text form of a value, used by out() and by bare-expression printing.
std::string toText(const Value& v);
// Structural equality, used by eq() / neq().
bool valuesEqual(const Value& a, const Value& b);
