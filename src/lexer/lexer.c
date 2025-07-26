// lexer.c

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer/lexer.h"
#include "utils/utils.h"

// === Helpers === //
char peek(Lexer *lexer)
{
    if (lexer->pos >= lexer->length)
        return '\0';
    return lexer->source[lexer->pos];
}

char advance(Lexer *lexer)
{
    return lexer->source[lexer->pos++];
}

bool match(Lexer *lexer, char expected)
{
    if (peek(lexer) == expected)
    {
        lexer->pos++;
        return true;
    }
    return false;
}

void skip_multiline_comment(Lexer *lexer)
{
    while (lexer->pos < lexer->length)
    {
        if (peek(lexer) == '#' && lexer->pos + 1 < lexer->length && lexer->source[lexer->pos + 1] == '#')
        {
            lexer->pos += 2; // Skip the closing ##
            return;
        }
        advance(lexer);
    }

    log_error("Unterminated multiline comment");
    exit(1);
}

// === Token Creation === //
Token make_token(TokenType type, const char *start, size_t len)
{
    Token token;
    token.type = type;
    token.value = strndup(start, len);
    return token;
}

void lexer_init(Lexer *lexer, const char *source)
{
    lexer->source = source;
    lexer->pos = 0;
    lexer->length = strlen(source);
    lexer->indent_stack[0] = 0;
    lexer->indent_top = 0;
    lexer->pending_dedents = 0;
    lexer->at_line_start = 1;
}

// === Core Tokenizer === //
Token next_token(Lexer *lexer)
{
    if (lexer->pending_dedents > 0)
    {
        lexer->pending_dedents--;
        return make_token(TOKEN_DEDENT, "", 0);
    }

    if (lexer->at_line_start)
    {
        int indent = 0;
        while (peek(lexer) == ' ')
        {
            advance(lexer);
            indent++;
        }

        if (peek(lexer) == '\n')
        {
            advance(lexer);
            return make_token(TOKEN_NEWLINE, "\n", 1);
        }

        if (indent > lexer->indent_stack[lexer->indent_top])
        {
            lexer->indent_top++;
            lexer->indent_stack[lexer->indent_top] = indent;
            lexer->at_line_start = 0;
            return make_token(TOKEN_INDENT, "", 0);
        }

        if (indent < lexer->indent_stack[lexer->indent_top])
        {
            while (indent < lexer->indent_stack[lexer->indent_top])
            {
                lexer->indent_top--;
                lexer->pending_dedents++;
            }
            lexer->at_line_start = 0;
            lexer->pending_dedents--; /* return one dedent now */
            return make_token(TOKEN_DEDENT, "", 0);
        }

        lexer->at_line_start = 0;
    }

    while (1)
    {
        char c = peek(lexer);

        // Skip whitespace
        if (c == ' ' || c == '\t' || c == '\r')
        {
            advance(lexer);
            continue;
        }
        if (c == '\n')
        {
            advance(lexer);
            lexer->at_line_start = 1;
            return make_token(TOKEN_NEWLINE, "\n", 1);
        }

        // Single-line comment
        if (c == '#' && lexer->source[lexer->pos + 1] != '#')
        {
            while (peek(lexer) != '\n' && peek(lexer) != '\0')
                advance(lexer);
            continue;
        }

        // Multiline comment ##
        if (c == '#' && lexer->pos + 1 < lexer->length && lexer->source[lexer->pos + 1] == '#')
        {
            lexer->pos += 2; // Skip initial ##
            skip_multiline_comment(lexer);
            continue;
        }

        break;
    }

    char c = advance(lexer);

    if (c == '\0')
    {
        if (lexer->indent_top > 0)
        {
            lexer->indent_top--;
            return make_token(TOKEN_DEDENT, "", 0);
        }
        return make_token(TOKEN_EOF, "", 0);
    }

    // Commands
    if (isalpha(c))
    {
        const char *start = &lexer->source[lexer->pos - 1];
        while (isalnum(peek(lexer)) || peek(lexer) == '_')
            advance(lexer);

        size_t len = &lexer->source[lexer->pos] - start;

        if (len == 3 && strncmp(start, "set", len) == 0)
            return make_token(TOKEN_SET, start, len);
        if (len == 2 && strncmp(start, "to", len) == 0)
            return make_token(TOKEN_TO, start, len);
        if (len == 2 && strncmp(start, "pr", len) == 0)
            return make_token(TOKEN_IDENTIFIER, start, len);
        if (len == 3 && strncmp(start, "GET", len) == 0)
            return make_token(TOKEN_GET, start, len);
        if (len == 4 && strncmp(start, "POST", len) == 0)
            return make_token(TOKEN_POST, start, len);
        if (len == 6 && strncmp(start, "return", len) == 0)
            return make_token(TOKEN_RETURN, start, len);
        if (len == 4 && strncmp(start, "true", len) == 0)
            return make_token(TOKEN_TRUE, start, len);
        if (len == 5 && strncmp(start, "false", len) == 0)
            return make_token(TOKEN_FALSE, start, len);
        if (len == 4 && strncmp(start, "null", len) == 0)
            return make_token(TOKEN_NULL, start, len);

        return make_token(TOKEN_IDENTIFIER, start, len);
    }

    // Numbers
    if (isdigit(c))
    {
        const char *start = &lexer->source[lexer->pos - 1];
        while (isdigit(peek(lexer)))
            advance(lexer);
        return make_token(TOKEN_NUMBER, start, &lexer->source[lexer->pos] - start);
    }

    // Strings
    if (c == '"')
    {
        const char *start = &lexer->source[lexer->pos];
        while (peek(lexer) != '"' && peek(lexer) != '\0')
            advance(lexer);

        size_t len = &lexer->source[lexer->pos] - start;
        match(lexer, '"');
        return make_token(TOKEN_STRING, start, len);
    }

    // General operators
    if (c == '=')
    {
        if (match(lexer, '='))
        {
            if (match(lexer, '='))
                return make_token(TOKEN_STRICT_EQ, "===", 3);
            return make_token(TOKEN_EQ, "==", 2);
        }
        return make_token(TOKEN_ASSIGN, "=", 1);
    }
    if (c == '{')
        return make_token(TOKEN_LBRACE, "{", 1);
    if (c == '}')
        return make_token(TOKEN_RBRACE, "}", 1);
    if (c == ':')
        return make_token(TOKEN_COLON, ":", 1);
    if (c == ',')
        return make_token(TOKEN_COMMA, ",", 1);
    if (c == '(')
        return make_token(TOKEN_LPAREN, "(", 1);
    if (c == ')')
        return make_token(TOKEN_RPAREN, ")", 1);
    if (c == '.')
        return make_token(TOKEN_DOT, ".", 1);
    if (c == '+')
        return make_token(TOKEN_PLUS, "+", 1);
    if (c == '-' && match(lexer, '>'))
        return make_token(TOKEN_ARROW, "->", 2);
    if (c == '-')
        return make_token(TOKEN_MINUS, "-", 1);
    if (c == '*')
        return make_token(TOKEN_STAR, "*", 1);
    if (c == '%')
        return make_token(TOKEN_PERCENT, "%", 1);
    if (c == '/')
        return make_token(TOKEN_SLASH, "/", 1);

    return make_token(TOKEN_UNKNOWN, &c, 1);
}
