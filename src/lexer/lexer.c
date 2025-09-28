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
Token make_token(TokenType type, const char *start, size_t len, int line, int column)
{
    Token token;
    token.type = type;
    token.value = strndup(start, len);
    token.line = line;
    token.column = column;
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
    lexer->line = 1;
    lexer->line_start = 0;
}

// === Core Tokenizer === //
Token next_token(Lexer *lexer)
{
    if (lexer->pending_dedents > 0)
    {
        lexer->pending_dedents--;
        return make_token(TOKEN_DEDENT, "", 0, lexer->line, 1);
    }

    if (lexer->at_line_start)
    {
        while (1)
        {
            int indent = 0;
            while (peek(lexer) == ' ' || peek(lexer) == '\t')
            {
                advance(lexer);
                indent++;
            }

            char c = peek(lexer);

            if (c == '\n')
            {
                advance(lexer);
                lexer->line++;
                lexer->line_start = lexer->pos;
                continue; /* skip blank line */
            }

            if (c == '#' && lexer->source[lexer->pos + 1] != '#')
            {
                while (peek(lexer) != '\n' && peek(lexer) != '\0')
                    advance(lexer);
                continue; /* skip comment line */
            }

            if (c == '#' && lexer->pos + 1 < lexer->length &&
                lexer->source[lexer->pos + 1] == '#')
            {
                lexer->pos += 2; /* skip initial ## */
                skip_multiline_comment(lexer);
                continue; /* multiline comment may span lines */
            }

            if (indent > lexer->indent_stack[lexer->indent_top])
            {
                lexer->indent_top++;
                lexer->indent_stack[lexer->indent_top] = indent;
                lexer->at_line_start = 0;
                return make_token(TOKEN_INDENT, "", 0, lexer->line, 1);
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
                return make_token(TOKEN_DEDENT, "", 0, lexer->line, 1);
            }

            lexer->at_line_start = 0;
            break;
        }
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
            lexer->line++;
            lexer->line_start = lexer->pos;
            return make_token(TOKEN_NEWLINE, "\n", 1, lexer->line - 1, 1);
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
    size_t start_pos = lexer->pos - 1;
    int column = (int)(start_pos - lexer->line_start) + 1;

    if (c == '\0')
    {
        if (lexer->indent_top > 0)
        {
            lexer->indent_top--;
            return make_token(TOKEN_DEDENT, "", 0, lexer->line, column);
        }
        return make_token(TOKEN_EOF, "", 0, lexer->line, column);
    }

    // Commands
    if (isalpha(c))
    {
        const char *start = &lexer->source[lexer->pos - 1];
        while (isalnum(peek(lexer)) || peek(lexer) == '_')
            advance(lexer);

        size_t len = &lexer->source[lexer->pos] - start;

        if (len == 3 && strncmp(start, "fun", len) == 0)
            return make_token(TOKEN_FUN, start, len, lexer->line, column);
        if (len == 2 && strncmp(start, "if", len) == 0)
            return make_token(TOKEN_IF, start, len, lexer->line, column);
        if (len == 4 && strncmp(start, "elif", len) == 0)
            return make_token(TOKEN_ELIF, start, len, lexer->line, column);
        if (len == 4 && strncmp(start, "else", len) == 0)
            return make_token(TOKEN_ELSE, start, len, lexer->line, column);
        if (len == 5 && strncmp(start, "class", len) == 0)
            return make_token(TOKEN_CLASS, start, len, lexer->line, column);
        if (len == 3 && strncmp(start, "for", len) == 0)
            return make_token(TOKEN_FOR, start, len, lexer->line, column);
        if (len == 2 && strncmp(start, "of", len) == 0)
            return make_token(TOKEN_OF, start, len, lexer->line, column);
        if (len == 5 && strncmp(start, "while", len) == 0)
            return make_token(TOKEN_WHILE, start, len, lexer->line, column);
        if (len == 5 && strncmp(start, "break", len) == 0)
            return make_token(TOKEN_BREAK, start, len, lexer->line, column);
        if (len == 8 && strncmp(start, "continue", len) == 0)
            return make_token(TOKEN_CONTINUE, start, len, lexer->line, column);
        if (len == 3 && strncmp(start, "and", len) == 0)
            return make_token(TOKEN_AND, start, len, lexer->line, column);
        if (len == 2 && strncmp(start, "or", len) == 0)
            return make_token(TOKEN_OR, start, len, lexer->line, column);
        if (len == 3 && strncmp(start, "not", len) == 0)
            return make_token(TOKEN_NOT, start, len, lexer->line, column);
        if (len == 6 && strncmp(start, "import", len) == 0)
            return make_token(TOKEN_IMPORT, start, len, lexer->line, column);
        if (len == 4 && strncmp(start, "from", len) == 0)
            return make_token(TOKEN_FROM, start, len, lexer->line, column);
        if (len == 2 && strncmp(start, "pr", len) == 0)
            return make_token(TOKEN_IDENTIFIER, start, len, lexer->line, column);
        if (len == 3 && strncmp(start, "GET", len) == 0)
            return make_token(TOKEN_GET, start, len, lexer->line, column);
        if (len == 4 && strncmp(start, "POST", len) == 0)
            return make_token(TOKEN_POST, start, len, lexer->line, column);
        if (len == 6 && strncmp(start, "return", len) == 0)
            return make_token(TOKEN_RETURN, start, len, lexer->line, column);
        if (len == 4 && strncmp(start, "true", len) == 0)
            return make_token(TOKEN_TRUE, start, len, lexer->line, column);
        if (len == 5 && strncmp(start, "false", len) == 0)
            return make_token(TOKEN_FALSE, start, len, lexer->line, column);
        if (len == 4 && strncmp(start, "null", len) == 0)
            return make_token(TOKEN_NULL, start, len, lexer->line, column);

        return make_token(TOKEN_IDENTIFIER, start, len, lexer->line, column);
    }

    // Numbers
    if (isdigit(c))
    {
        const char *start = &lexer->source[lexer->pos - 1];
        while (isdigit(peek(lexer)))
            advance(lexer);
        return make_token(TOKEN_NUMBER, start, &lexer->source[lexer->pos] - start, lexer->line, column);
    }

    if (c == '@')
    {
        const char *start = &lexer->source[lexer->pos];
        while (isalnum(peek(lexer)))
            advance(lexer);
        size_t len = &lexer->source[lexer->pos] - start;
        if (len == 6 && strncmp(start, "static", len) == 0)
            return make_token(TOKEN_AT_STATIC, &lexer->source[start_pos], len + 1, lexer->line, column);
        if (len == 7 && strncmp(start, "private", len) == 0)
            return make_token(TOKEN_AT_PRIVATE, &lexer->source[start_pos], len + 1, lexer->line, column);
        return make_token(TOKEN_UNKNOWN, "@", 1, lexer->line, column);
    }

    // Strings
    if (c == '"')
    {
        const char *start = &lexer->source[lexer->pos];
        while (peek(lexer) != '"' && peek(lexer) != '\0')
            advance(lexer);

        size_t len = &lexer->source[lexer->pos] - start;
        match(lexer, '"');
        return make_token(TOKEN_STRING, start, len, lexer->line, column);
    }

    // General operators
    if (c == '=')
    {
        if (match(lexer, '='))
        {
            if (match(lexer, '='))
                return make_token(TOKEN_STRICT_EQ, "===", 3, lexer->line, column);
            return make_token(TOKEN_EQ, "==", 2, lexer->line, column);
        }
        return make_token(TOKEN_ASSIGN, "=", 1, lexer->line, column);
    }
    if (c == '<')
    {
        if (match(lexer, '='))
            return make_token(TOKEN_LTE, "<=", 2, lexer->line, column);
        return make_token(TOKEN_LT, "<", 1, lexer->line, column);
    }
    if (c == '>')
    {
        if (match(lexer, '='))
            return make_token(TOKEN_GTE, ">=", 2, lexer->line, column);
        return make_token(TOKEN_GT, ">", 1, lexer->line, column);
    }
    if (c == '+')
    {
        if (match(lexer, '+'))
            return make_token(TOKEN_INC, "++", 2, lexer->line, column);
    }
    if (c == '-' && match(lexer, '>'))
        return make_token(TOKEN_ARROW, "->", 2, lexer->line, column);

    static const struct
    {
        char ch;
        TokenType type;
    } single_char_tokens[] = {
        {'[', TOKEN_LBRACKET},
        {']', TOKEN_RBRACKET},
        {'{', TOKEN_LBRACE},
        {'}', TOKEN_RBRACE},
        {':', TOKEN_COLON},
        {',', TOKEN_COMMA},
        {'(', TOKEN_LPAREN},
        {')', TOKEN_RPAREN},
        {'.', TOKEN_DOT},
        {'+', TOKEN_PLUS},
        {'-', TOKEN_MINUS},
        {'*', TOKEN_STAR},
        {'%', TOKEN_PERCENT},
        {'/', TOKEN_SLASH},
        {'?', TOKEN_QUESTION},
    };

    for (size_t i = 0; i < sizeof(single_char_tokens) / sizeof(single_char_tokens[0]); ++i)
    {
        if (c == single_char_tokens[i].ch)
            return make_token(single_char_tokens[i].type, &lexer->source[start_pos], 1, lexer->line, column);
    }

    return make_token(TOKEN_UNKNOWN, &c, 1, lexer->line, column);
}
