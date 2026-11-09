/*
The entry point. It reads the script file and passes the result of each stage to
the next: lexer, then parser, then analyser, then interpreter. It catches PunchError 
and prints it with its stage and line number. It also supports --tokens, --ast 
and --check, which stop after the lexer, parser or analyser so each stage can 
be inspected on its own. It does no language work itself.
*/

// FILES AND FUNCTIONS GOES AS FOLLOWS: script.punch -> lexer -> tokens -> parser
// -> AST -> Analyser -> Interpereter -> output

int main() { 

return 0;

}
