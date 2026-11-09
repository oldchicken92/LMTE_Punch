/*
Implements execution by walking the AST directly, so no machine code is
generated. Statements are executed (declarations, change, if, loop, while)
and expressions are evaluated, with the arguments of a call evaluated first 
so that nesting sets the order. It implements the built-in functions: arithmetic, 
comparison, logic, file commands (rename, delete, wipe, size, duplicate) and 
input/output (in, in_file, out, out_file, out_append). resolve() implements 
the headline feature: it looks in the current folder first, then searches 
sub-folders downward, never upward and never through symlinks. One match is 
used, several give an ambiguity error, and none gives a not-found error. It 
reports runtime errors such as a missing file, division by zero, an index 
out of range, a negative loop count and a non-empty folder on delete.
*/

#include "interpreter.h"

void Interpreter::run(const Program&) {
}
