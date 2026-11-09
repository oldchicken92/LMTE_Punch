/*
Declares the Interpreter class, a tree-walking interpreter. It holds the 
variable memory (env, a map from name to current value), the exec functions 
for statements and the eval functions for expressions. It also declares the 
groups of built-in functions and resolve(), which implements the downward file search.
*/

#pragma once
#include "ast.h"
class Interpreter {
public:
    void run(const Program& program);
};
