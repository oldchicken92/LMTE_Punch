/*
Declares the Analyser class, which does the semantic checks. It lists the 
symbol table (a map from variable name to type), the statement checks, 
the typeOf functions and the helpers.
*/

#pragma once
#include "ast.h"
class Analyser {
public:
    void analyse(const Program& program);
};
