/*
Holds the two type systems of Punch. The static types (Kind and Type) describe what 
an expression is before the program runs, and the analyser uses them. The 
runtime Value is a std::variant that holds one of no value, number, boolean, string, 
Path (a file or folder) or list, and the interpreter uses it. It also declares helper
functions for converting types and values to text.
*/

#pragma once