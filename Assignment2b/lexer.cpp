/*
Implements the lexer as a finite state machine. nextToken() looks at the first 
character and chooses the reader: " for a string, - for a number or variable, 
a digit for a number, a letter for a word, and anything else for punctuation. 
Words are classified as folder, file, boolean, function or keyword, and a type 
word is read straight after a variable. It skips spaces and comments, collapses 
blank lines into one NEWLINE, tracks line numbers and reports lexical errors 
such as an invalid character, an unterminated string, an unclosed variable 
and an unknown word.
*/

#include "lexer.h"
#include <cctype>
