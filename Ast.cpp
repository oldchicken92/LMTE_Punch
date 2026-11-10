#include "Ast.h"
#include <ostream>

// ============================================================================
// ast.cpp  -  AST pretty-printing helpers
//
// This file is not part of execution. Instead, it reconstructs a source-like
// form from the parsed nodes so developers can inspect the tree with
// ./punch --ast script.punch.
//
// It is mainly a debugging aid: it turns the structured AST back into a readable
// approximation of the original source, with indentation showing block nesting.
// This helps when a parser bug or semantic bug needs a human-readable snapshot.
// ============================================================================

// ---------------------------------------------------------------------------
// exprToString: turn an expression node back into readable text.
// Handy for seeing exactly what the parser understood.
// ---------------------------------------------------------------------------
std::string exprToString(Expr& e) {
    if (auto* n = dynamic_cast<NumberLit*>(&e)) return toText(Value(n->value));
    if (auto* s = dynamic_cast<StringLit*>(&e)) return "\"" + s->value + "\"";
    if (auto* b = dynamic_cast<BoolLit*>(&e)) return b->value ? "true" : "false";
    if (auto* f = dynamic_cast<FileLit*>(&e)) return f->text;
    if (auto* f = dynamic_cast<FolderLit*>(&e)) return f->text;

    if (auto* l = dynamic_cast<ListLit*>(&e)) {
        std::string out = "[";
        for (size_t i = 0; i < l->items.size(); i++) {
            if (i > 0) out += ", ";
            out += exprToString(*l->items[i]);
        }
        return out + "]";
    }
    if (auto* v = dynamic_cast<VarRef*>(&e)) {
        std::string out = "-" + v->name + "-";
        if (v->index) out += "[" + exprToString(*v->index) + "]";
        return out;
    }
    if (auto* c = dynamic_cast<Call*>(&e)) {
        std::string out = c->name + "(";
        for (size_t i = 0; i < c->args.size(); i++) {
            if (i > 0) out += ", ";
            out += exprToString(*c->args[i]);
        }
        return out + ")";
    }
    return "?";
}

// ---------------------------------------------------------------------------
// printProgram: indented dump of the statement tree.
// ---------------------------------------------------------------------------
namespace {
void printBlock(Block& block, std::ostream& os, int depth);

void printStatement(Stmt& s, std::ostream& os, int depth) {
    std::string pad(depth * 2, ' ');

    if (auto* d = dynamic_cast<Declaration*>(&s)) {
        os << pad << "Declaration -" << d->name << "- : " << typeName(d->type)
           << " = " << exprToString(*d->value) << "\n";
    } else if (auto* c = dynamic_cast<Change*>(&s)) {
        os << pad << "Change " << exprToString(*c->target) << " <- "
           << exprToString(*c->value) << "\n";
    } else if (auto* i = dynamic_cast<If*>(&s)) {
        os << pad << "If " << exprToString(*i->condition) << "\n";
        printBlock(i->thenBlock, os, depth + 1);
        if (!i->elseBlock.empty()) {
            os << pad << "Else\n";
            printBlock(i->elseBlock, os, depth + 1);
        }
    } else if (auto* l = dynamic_cast<Loop*>(&s)) {
        os << pad << "Loop " << exprToString(*l->count) << " times\n";
        printBlock(l->body, os, depth + 1);
    } else if (auto* w = dynamic_cast<While*>(&s)) {
        os << pad << "While " << exprToString(*w->condition) << "\n";
        printBlock(w->body, os, depth + 1);
    } else if (auto* e = dynamic_cast<ExprStmt*>(&s)) {
        os << pad << "Expression " << exprToString(*e->expr) << "\n";
    }
}

void printBlock(Block& block, std::ostream& os, int depth) {
    for (StmtPtr& s : block) printStatement(*s, os, depth);
}
}  // namespace

void printProgram(Program& program, std::ostream& os) {
    printBlock(program.statements, os, 0);
}
