/*
Implements the helpers declared in value.h. kindName() and typeName() give type names
for error messages, and parseTypeName() turns a source word like list_string into a Type. 
toText() converts a value to the text that out() prints (numbers without trailing zeros, 
lists as [a, b]), and valuesEqual() implements eq and neq.
*/

#include "value.h"
#include <iomanip>
#include <sstream>
