#include "Ast.h"
#include <ostream>

// NOT part of execution, instead reconstructs a source-like
// form from the parsed nodes so we can inspect the tree with
// ./punch --ast script.punch FLAG


// exprToString: turn an expression node back into readable text
// Useful for seeing exactly what the parser understood
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


// printProgram: indented dump of the statement tree.
namespace {
// forward declaration required for recursive block traversal inside nested statements
void printBlock(Block& block, std::ostream& os, int depth);

// inspects a single statement node via dynamic RTTI casting and prints it out
void printStatement(Stmt& s, std::ostream& os, int depth) {
    std::string pad(depth * 2, ' ');

    // use dynamic_cast to determine the statement type
    if (auto* d = dynamic_cast<Declaration*>(&s)) {
        os << pad << "Declaration -" << d->name << "- : " << typeName(d->type)
           << " = " << exprToString(*d->value) << "\n";
    } else if (auto* c = dynamic_cast<Change*>(&s)) {
        os << pad << "Change " << exprToString(*c->target) << " <- "
           << exprToString(*c->value) << "\n";
    } else if (auto* i = dynamic_cast<If*>(&s)) {
        os << pad << "If " << exprToString(*i->condition) << "\n";
        // recursively print nested statements inside the 'then' block
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
// iterates through a vector of statement unique_ptrs (Block) sequentially
void printBlock(Block& block, std::ostream& os, int depth) {
    for (StmtPtr& s : block) printStatement(*s, os, depth);
}
}  // namespace

// called by main.cpp when using --ast flag
void printProgram(Program& program, std::ostream& os) {
    printBlock(program.statements, os, 0);
}
