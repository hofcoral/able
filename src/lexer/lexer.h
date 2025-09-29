#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>

#include "types/value.h"

typedef enum
{
    TOKEN_EOF,
    TOKEN_FUN,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_IMPORT,
    TOKEN_FROM,
    TOKEN_ASSIGN,
    TOKEN_GET,
    TOKEN_POST,
    TOKEN_PUT,
    TOKEN_PATCH,
    TOKEN_DELETE,
    TOKEN_HEAD,
    TOKEN_OPTIONS,
    TOKEN_RETURN,
    TOKEN_ASYNC,
    TOKEN_AWAIT,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_CLASS,
    TOKEN_FOR,
    TOKEN_OF,
    TOKEN_WHILE,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_PLUS,
    TOKEN_INC,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_QUESTION,
    TOKEN_EQ,
    TOKEN_STRICT_EQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTE,
    TOKEN_GTE,
    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_AT_STATIC,
    TOKEN_AT_PRIVATE,
    TOKEN_UNKNOWN
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
    int line;
    int column;
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
    int line;
    size_t line_start;
} Lexer;

Token next_token(Lexer *lexer);
void lexer_init(Lexer *lexer, const char *source);
#endif
