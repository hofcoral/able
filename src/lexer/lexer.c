// lexer.c

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer/lexer.h"

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

// === Token Creation === //
Token make_token(TokenType type, const char *start, size_t len)
{
    Token token;
    token.type = type;
    token.value = strndup(start, len);
    return token;
}

// === Core Tokenizer === //
Token next_token(Lexer *lexer)
{
    while (1)
    {
        char c = peek(lexer);

        // Skip whitespace
        if (isspace(c))
        {
            if (c == '\n')
            {
                advance(lexer);
                return make_token(TOKEN_NEWLINE, "\n", 1);
            }
            advance(lexer);
            continue;
        }

        // Single-line comment
        if (c == '#' && lexer->source[lexer->pos + 1] != '#')
        {
            while (peek(lexer) != '\n' && peek(lexer) != '\0')
                advance(lexer);
            continue;
        }

        // Multiline comment ##
        if (c == '#' && lexer->source[lexer->pos + 1] == '#')
        {
            lexer->pos += 2; // Skip initial ##
            while (!(peek(lexer) == '#' && lexer->source[lexer->pos + 1] == '#'))
            {
                if (peek(lexer) == '\0')
                {
                    fprintf(stderr, "Unterminated multiline comment\n");
                    exit(1);
                }
                advance(lexer);
            }
            lexer->pos += 2; // Skip closing ##
            continue;
        }

        break;
    }

    char c = advance(lexer);

    // EOF
    if (c == '\0')
        return make_token(TOKEN_EOF, "", 0);

    // Commands
    if (isalpha(c))
    {
        const char *start = &lexer->source[lexer->pos - 1];
        while (isalnum(peek(lexer)) || peek(lexer) == '_')
            advance(lexer);

        size_t len = &lexer->source[lexer->pos] - start;

        if (strncmp(start, "set", len) == 0)
            return make_token(TOKEN_SET, start, len);
        if (strncmp(start, "to", len) == 0)
            return make_token(TOKEN_TO, start, len);
        if (strncmp(start, "pr", len) == 0)
            return make_token(TOKEN_IDENTIFIER, start, len);
        if (strncmp(start, "GET", len) == 0)
            return make_token(TOKEN_GET, start, len);
        if (strncmp(start, "POST", len) == 0)
            return make_token(TOKEN_POST, start, len);

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
        return make_token(TOKEN_ASSIGN, "=", 1);
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

    return make_token(TOKEN_UNKNOWN, &c, 1);
}
