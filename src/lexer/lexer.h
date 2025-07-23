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
    TOKEN_ASSIGN,
    TOKEN_GET,
    TOKEN_POST,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_NEWLINE,
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
} Lexer;

Token next_token(Lexer *lexer);
#endif
