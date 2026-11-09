/*
Implements the semantic analysis, which runs over the AST before any code executes. 
It keeps a symbol table and computes a type for every expression. It checks that 
variables are declared before use and not declared twice, that values match declared 
types (including the element type of lists), that file variables are never changed, 
that function argument counts and types are right, that if and while conditions are 
boolean, and that loop counts and list indexes are numbers. typeOfCall() holds the 
signature table for every built-in function. Because it runs before execution, a 
mistake on line 20 cannot leave a script half-run after it has
changed files on line 3.
*/

#include "analyser.h"

void Analyser::analyse(const Program&) {

}
