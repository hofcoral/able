#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "types/value.h"

typedef enum
{
    TOKEN_EOF,
    TOKEN_SET,
    TOKEN_TO,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_ASSIGN,
    TOKEN_GET,
    TOKEN_POST,
    TOKEN_RETURN,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_UNKNOWN
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
} Token;

typedef struct
{
    const char *source;
    size_t pos;
    size_t length;
    int indent_stack[64];
    int indent_top;
    int pending_dedents;
    int at_line_start;
} Lexer;

Token next_token(Lexer *lexer);
void lexer_init(Lexer *lexer, const char *source);
#endif
