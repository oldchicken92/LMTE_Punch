#pragma once
// ============================================================================
// errors.h  -  shared exception type for every compiler stage
//
// The language pipeline has several failure points: lexical analysis, syntax
// parsing, semantic analysis, and runtime execution. All of them throw the same
// PunchError type so the CLI can print a consistent message with a stage name
// and line number.
// ============================================================================
#include <stdexcept>
#include <string>

// Every compiler stage throws the same error type so the CLI can report a
// consistent message that includes the stage name and source line.
struct PunchError : std::runtime_error {
    std::string stage; // lexical, syntax, semantic, or runtime
    int line;          // source line where the problem was detected

    // The constructor stores the phase and location so every compiler stage can
    // report failures in a consistent, user-friendly format.
    PunchError(const std::string& errorStage, int sourceLine, const std::string& errorMessage)
    : std::runtime_error("[" + errorStage + " error, line " +
    std::to_string(sourceLine) + "] " + errorMessage), stage(errorStage), line(sourceLine) {}
};
