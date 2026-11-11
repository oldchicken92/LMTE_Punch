#include "Token.h"

std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::Variable:  return "VARIABLE";
        case TokenType::Type:      return "TYPE";
        case TokenType::Number:    return "NUMBER";
        case TokenType::String:    return "STRING";
        case TokenType::Boolean:   return "BOOLEAN";
        case TokenType::File:      return "FILE";
        case TokenType::Folder:    return "FOLDER";
        case TokenType::Function:  return "FUNCTION";
        case TokenType::Keyword:   return "KEYWORD";
        case TokenType::LParen:    return "LPAREN";
        case TokenType::RParen:    return "RPAREN";
        case TokenType::LBracket:  return "LBRACKET";
        case TokenType::RBracket:  return "RBRACKET";
        case TokenType::Comma:     return "COMMA";
        case TokenType::Equals:    return "EQUALS";
        case TokenType::Newline:   return "NEWLINE";
        case TokenType::EndOfFile: return "EOF";
    }
    return "UNKNOWN";
}
