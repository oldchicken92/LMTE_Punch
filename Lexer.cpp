/*
lexer.cpp turns raw source text into Punch tokens

The lexer follows a small finite-state design: it reads one character at a
time, decides what kind of token it is looking at, and delegates to a helper
such as readNumber(), readString(), or readVariable(). The goal is to turn a
script into a predictable stream the parser can consume without worrying about
whitespace, comments, or punctuation details.

Important idea: tokenization is the "translation layer" between the user's
source file and the grammar. The parser never has to think about raw characters
such as whitespace or quote marks; it just consumes clean, typed tokens.
*/

#include "Lexer.h"
#include <cctype>

namespace {
// character classes
bool isDigit(char c)     { return std::isdigit(static_cast<unsigned char>(c)); }
bool isLetter(char c)    { return std::isalpha(static_cast<unsigned char>(c)); }
bool isNameStart(char c) { return isLetter(c) || c == '_'; }
bool isNameChar(char c)  { return isNameStart(c) || isDigit(c); }
bool isWordChar(char c)  { return isNameChar(c) || c == '.' || c == '/'; }

// Valid type names such as number, string, file, and list_string.
bool isBaseType(const std::string& typeName) {
    return typeName == "number" || typeName == "string" || typeName == "boolean" ||
           typeName == "file"   || typeName == "folder";
}

bool isValidType(const std::string& typeName) {
    if (isBaseType(typeName) || typeName == "list") return true;
    return typeName.rfind("list_", 0) == 0 && isBaseType(typeName.substr(5));
}

// Built-in function names recognized by the language.
bool isFunctionName(const std::string& functionName) {
    static const char* names[] = {
        "add", "sub", "mult", "div", "lt", "gt", "eq", "neq", "and", "or",
        "not", "rename", "delete", "wipe", "size", "duplicate",
        "in", "in_file", "out", "out_file", "out_append"};
    for (const char* functionText : names) if (functionName == functionText) return true;
    return false;
}

// Keywords that begin or end statement blocks in the Punch language.
bool isBlockKeyword(const std::string& keywordText) {
    static const char* names[] = {
        "if", "else", "endif", "loop", "endloop",
        "while", "endwhile", "change"};
    for (const char* keyword : names) if (keywordText == keyword) return true;
    return false;
}

// Detect names that look like file paths, such as report.txt or data.json.
bool looksLikeFile(const std::string& possibleFileName) {
    size_t dotPosition = possibleFileName.rfind('.');
    return dotPosition != std::string::npos && dotPosition > 0 && dotPosition + 1 < possibleFileName.size();
}
}  // namespace

Lexer::Lexer(const std::string& source) : src(source) {}

// Track the current reading position so we can safely move through the source.
bool Lexer::atEnd() const { return pos >= src.size(); }

char Lexer::peek(size_t offset) const {
    return pos + offset < src.size() ? src[pos + offset] : '\0';
}

char Lexer::advance() { return src[pos++]; }

// Create a token with the current line number attached for better error reporting.
Token Lexer::makeToken(TokenType type, const std::string& text) const {
    return Token{type, text, line};
}

PunchError Lexer::error(const std::string& message) const {
    return PunchError("lexical", line, message);
}

// Ignore spaces, tabs, and carriage returns; they do not affect the language.
void Lexer::skipSpaces() {
    while (peek() == ' ' || peek() == '\t' || peek() == '\r') advance();
}

// A double slash begins a comment, so everything up to the next newline is ignored.
bool Lexer::isCommentStart() const { return peek() == '/' && peek(1) == '/'; }

void Lexer::skipComment() {
    while (!atEnd() && peek() != '\n') advance();
}

// Newlines split statements. We emit a single newline token for each real line break.
void Lexer::handleNewline(std::vector<Token>& tokens) {
    if (!tokens.empty() && tokens.back().type != TokenType::Newline)
        tokens.push_back(makeToken(TokenType::Newline, "\\n"));
    advance();
    line++;
}

// Main scanning loop: keep reading until the source is exhausted.
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipSpaces();
        if (atEnd()) break;
        if (isCommentStart()) { skipComment(); continue; }
        if (peek() == '\n')   { handleNewline(tokens); continue; }

        Token token = nextToken();
        tokens.push_back(token);

        // Variable declarations can be followed by a type word such as
        // -num-number, which the parser reads as part of the same declaration.
        if (token.type == TokenType::Variable && isLetter(peek()))
            tokens.push_back(readType());
    }

    if (!tokens.empty() && tokens.back().type != TokenType::Newline)
        tokens.push_back(makeToken(TokenType::Newline, "\\n"));
    tokens.push_back(makeToken(TokenType::EndOfFile, ""));
    return tokens;
}

// Decide which token reader should handle the next character.
Token Lexer::nextToken() {
    if (peek() == '"')    return readString();
    if (peek() == '-')    return readDash();
    if (isDigit(peek()))  return readNumber();
    if (isLetter(peek())) return readWord();
    return readPunctuation();
}

// Read a single punctuation symbol such as parentheses, brackets, commas, and equals.
Token Lexer::readPunctuation() {
    char punctuation = advance();
    switch (punctuation) {
        case '(': return makeToken(TokenType::LParen, "(");
        case ')': return makeToken(TokenType::RParen, ")");
        case '[': return makeToken(TokenType::LBracket, "[");
        case ']': return makeToken(TokenType::RBracket, "]");
        case ',': return makeToken(TokenType::Comma, ",");
        case '=': return makeToken(TokenType::Equals, "=");
        default:
            throw error(std::string("invalid character '") + punctuation + "'");
    }
}

// Read the numeric digits that make up an integer or decimal literal.
std::string Lexer::readDigits() {
    std::string digitText;
    while (isDigit(peek())) digitText += advance();
    return digitText;
}

Token Lexer::readNumber() {
    std::string numberText;
    if (peek() == '-') numberText += advance();
    numberText += readDigits();
    if (peek() == '.' && isDigit(peek(1))) {
        numberText += advance();
        numberText += readDigits();
    }
    return makeToken(TokenType::Number, numberText);
}

// Read a quoted string literal and reject unterminated text.
Token Lexer::readString() {
    advance();  // opening quote
    std::string stringContent;
    while (!atEnd() && peek() != '"' && peek() != '\n') stringContent += advance();
    if (atEnd() || peek() == '\n') throw error("unterminated string");
    advance();  // closing quote
    return makeToken(TokenType::String, stringContent);
}

// A leading '-' can either begin a negative number or open a variable name like -x-.
Token Lexer::readDash() {
    char nextCharacter = peek(1);
    if (isDigit(nextCharacter))     return readNumber();
    if (isNameStart(nextCharacter)) return readVariable();
    throw error("unexpected '-'");
}

// Read a variable declaration name such as -name-.
Token Lexer::readVariable() {
    advance();  // opening '-'
    std::string variableName;
    while (isNameChar(peek())) variableName += advance();
    if (peek() != '-')
        throw error("unclosed variable name '-" + variableName + "'");
    advance();  // closing '-'
    return makeToken(TokenType::Variable, variableName);
}

// Read the type name that follows a variable, like number or list_string.
Token Lexer::readType() {
    std::string typeName;
    while (isNameChar(peek())) typeName += advance();
    if (!isValidType(typeName)) throw error("unknown type '" + typeName + "'");
    return makeToken(TokenType::Type, typeName);
}

// Read a plain word like true, if, or docs/ and classify it into a token type.
Token Lexer::readWord() {
    std::string wordText;
    while (isWordChar(peek())) wordText += advance();
    return classifyWord(wordText);
}

Token Lexer::classifyWord(const std::string& wordText) const {
    if (wordText.back() == '/')                 return makeToken(TokenType::Folder, wordText);
    if (looksLikeFile(wordText))                return makeToken(TokenType::File, wordText);
    if (wordText == "true" || wordText == "false")  return makeToken(TokenType::Boolean, wordText);
    if (isFunctionName(wordText))               return makeToken(TokenType::Function, wordText);
    if (isBlockKeyword(wordText))               return makeToken(TokenType::Keyword, wordText);
    throw error("unknown word '" + wordText + "' (strings must be quoted)");
}
