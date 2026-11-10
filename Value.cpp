#include "Value.h"
#include <iomanip>
#include <sstream>

// ---------------------------------------------------------------------------
// Names for error messages
// ---------------------------------------------------------------------------
std::string kindName(Kind k) {
    switch (k) {
        case Kind::Number:  return "number";
        case Kind::String:  return "string";
        case Kind::Boolean: return "boolean";
        case Kind::File:    return "file";
        case Kind::Folder:  return "folder";
        case Kind::List:    return "list";
        case Kind::Void:    return "nothing";
        case Kind::Any:     return "any";
    }
    return "?";
}

std::string typeName(const Type& t) {
    if (t.kind == Kind::List && t.elem != Kind::Any) return "list_" + kindName(t.elem);
    return kindName(t.kind);
}

// ---------------------------------------------------------------------------
// Parsing a type word such as "number" or "list_string"
// ---------------------------------------------------------------------------
namespace {
// The five "base" types (the ones a list can hold).
bool baseKind(const std::string& w, Kind& k) {
    if (w == "number")  { k = Kind::Number;  return true; }
    if (w == "string")  { k = Kind::String;  return true; }
    if (w == "boolean") { k = Kind::Boolean; return true; }
    if (w == "file")    { k = Kind::File;    return true; }
    if (w == "folder")  { k = Kind::Folder;  return true; }
    return false;
}
}  // namespace

bool parseTypeName(const std::string& text, Type& out) {
    Kind k;
    if (baseKind(text, k)) { out = Type{k, Kind::Any}; return true; }
    if (text == "list")    { out = Type{Kind::List, Kind::Any}; return true; }
    if (text.rfind("list_", 0) == 0 && baseKind(text.substr(5), k)) {
        out = Type{Kind::List, k};
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Value -> text
// ---------------------------------------------------------------------------
std::string toText(const Value& v) {
    if (std::holds_alternative<std::monostate>(v.data)) return "";

    if (auto* d = std::get_if<double>(&v.data)) {
        // 12 significant digits: 65 prints as "65", 0.1+0.2 as "0.3".
        std::ostringstream os;
        os << std::setprecision(12) << *d;
        return os.str();
    }
    if (auto* b = std::get_if<bool>(&v.data))        return *b ? "true" : "false";
    if (auto* s = std::get_if<std::string>(&v.data)) return *s;
    if (auto* p = std::get_if<Path>(&v.data))        return p->p.string();

    // list: [a, b, c]
    const List& list = std::get<List>(v.data);
    std::string out = "[";
    for (size_t i = 0; i < list.size(); i++) {
        if (i > 0) out += ", ";
        out += toText(list[i]);
    }
    return out + "]";
}

// ---------------------------------------------------------------------------
// Equality
// ---------------------------------------------------------------------------
bool valuesEqual(const Value& a, const Value& b) {
    if (a.data.index() != b.data.index()) return false;   // different kinds are never equal

    if (auto* x = std::get_if<double>(&a.data))      return *x == std::get<double>(b.data);
    if (auto* x = std::get_if<bool>(&a.data))        return *x == std::get<bool>(b.data);
    if (auto* x = std::get_if<std::string>(&a.data)) return *x == std::get<std::string>(b.data);
    if (auto* x = std::get_if<Path>(&a.data))        return x->p == std::get<Path>(b.data).p;
    if (auto* x = std::get_if<List>(&a.data)) {
        const List& y = std::get<List>(b.data);
        if (x->size() != y.size()) return false;
        for (size_t i = 0; i < x->size(); i++)
            if (!valuesEqual((*x)[i], y[i])) return false;
        return true;
    }
    return true;   // both monostate
}
