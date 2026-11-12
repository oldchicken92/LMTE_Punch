/*
This is the main command-line centre for the Punch interperter
This reads the users source file and makes the compiler pipeline and decides
wher to stop based on the command line option chosen. 
Goes as follows: 
./punch script.punch            run the script
./punch --tokens script.punch   stop after the lexer, print the tokens
./punch --ast script.punch      stop after the parser, print the tree
./punch --check script.punch    stop after the analyser
*/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "Analyser.h"
#include "Errors.h"
#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"

// driver that decides which stage to run and goes to lexer, parser, analyser and interpereter

// main function to read file for operation
static std::string readFile(const std::string& filePath) {
    std::ifstream sourceFile(filePath); // reads files from computer
    if (!sourceFile) throw std::runtime_error("cannot open '" + filePath + "'"); // error incase file cannot be opened

    // FILe goes into string for buffer for lexer processing
    std::stringstream fileBuffer;
    fileBuffer << sourceFile.rdbuf();
    return fileBuffer.str();
}

// Main function -> parses cmd arguements and makes sure compiler/interpereter executes
int main(int argc, char* argv[]) {
    std::string executionMode = "run"; // default execution state
    std::string sourcePath; // path of script given by user

    // for loop and if statement for cmd options regarding toekn, ast or check
    for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex) {
        std::string argumentValue = argv[argumentIndex];
        if (argumentValue == "--tokens" || argumentValue == "--ast" || argumentValue == "--check")
            executionMode = argumentValue.substr(2);
        else
            sourcePath = argumentValue;
    }

    // error if there was no script/path given
    if (sourcePath.empty()) {
        std::cerr << "usage: punch [--tokens|--ast|--check] script.punch\n";
        return 1;
    }


    try {
        std::string sourceCode = readFile(sourcePath); // ensure source text/code is stored

        // First section: Characters -> tokens
        std::vector<Token> tokens = Lexer(sourceCode).tokenize(); 
        if (executionMode == "tokens") { // diagnostic mode which prints each token with line number
            for (const Token& token : tokens)
            std::cout << token.line << ": " << tokenTypeName(token.type) << "(" << token.text << ")\n";
            return 0;
        }

        // Section 2: Tokens -> syntax tree (for syntax errors)
        std::unique_ptr<Program> program = Parser(tokens).parseProgram(); // turns token stream to an AST
        if (executionMode == "ast") {  // mainly for debugging
            printProgram(*program, std::cout);
            return 0;
        }

        // Section 3: Semantic analysis (code makey sense)
        Analyser().analyse(*program); // checks program is valid
        if (executionMode == "check") { // checks to see if correct (testing mode)
            std::cout << "ok: no errors found\n";
            return 0;
        }

        // Last stage: execute program
        Interpreter().run(*program);

    } catch (const PunchError& error) {
        // as PunchError is used as a exception type, the error message is printed and has exit code
        std::cout.flush();
        std::cerr << error.what() << "\n";
        return 1;
    } catch (const std::exception& error) { // for unexpected runtime errors
        std::cout.flush();
        std::cerr << error.what() << "\n";
        return 1;
    }
    
    return 0;
}
