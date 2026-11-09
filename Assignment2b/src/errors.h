/*
Defines PunchError, the single error type used by every stage. It stores the 
stage (lexical, syntax, semantic or runtime), the line number and a message, and 
formats them as [syntax error, line 3] .... Because every stage throws this one 
type, main.cpp can catch and print any failure in one place.
*/

#pragma once
#include <stdexcept>
#include <string>

struct PunchError : std::runtime_error {
   
};
