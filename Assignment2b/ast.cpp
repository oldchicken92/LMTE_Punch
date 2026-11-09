/*
Implements the debug printers. exprToString() turns an expression back into 
source-like text, and printProgram() prints the whole tree indented. It is 
used by ./punch --ast to show what the parser understood.
*/

#include "ast.h"
#include <ostream>
