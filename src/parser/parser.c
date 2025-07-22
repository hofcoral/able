#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser/parser.h"
#include "types/value.h"
#include "lexer/lexer.h"
#include "types/object.h"

/* --- helpers --- */
static Token current;
static Lexer *L;

static void advance_token() { current = next_token(L); }

static int match(TokenType type)
{
    if (current.type == type)
    {
        advance_token();
        return 1;
    }
    return 0;
}

static void expect(TokenType type, const char *msg)
{
    if (!match(type))
    {
        fprintf(stderr, "Parse error: expected %s\n", msg);
        exit(1);
    }
}

/* --- AST constructors --- */
static ASTNode *new_node(NodeType t)
{
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = t;
    return n;
}

/* --- parsing functions --- */
static ASTNode *parse_set_stmt()
{
    ASTNode *n = new_node(NODE_SET);

    if (current.type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Expected identifier after 'set'\n");
        exit(1);
    }
    n->set_name = strdup(current.value);
    advance_token();

    if (current.type != TOKEN_TO)
    {
        fprintf(stderr, "Expected 'to' after variable name\n");
        exit(1);
    }
    advance_token();

    // Determine if assigning a literal or copying a variable
    if (current.type == TOKEN_STRING)
    {
        n->literal_value.type = VAL_STRING;
        n->literal_value.str = strdup(current.value);
        n->is_copy = false;
        advance_token();
    }
    else if (current.type == TOKEN_NUMBER)
    {
        n->literal_value.type = VAL_NUMBER;
        n->literal_value.num = atof(current.value);
        n->is_copy = false;
        advance_token();
    }
    else if (current.type == TOKEN_LBRACE)
    {
        ASTNode *obj_node = parse_object_literal();
        n->literal_value = obj_node->literal_value;
        n->is_copy = false;
        free(obj_node);
    }
    else if (current.type == TOKEN_IDENTIFIER)
    {
        n->copy_from_var = strdup(current.value);
        n->is_copy = true;
        advance_token();
    }
    else
    {
        fprintf(stderr, "Current: %s\n", current.value);

        fprintf(stderr, "Expected string, number, object, or identifier after 'to'\n");
        exit(1);
    }

    return n;
}

static ASTNode *parse_func_call()
{
    ASTNode *n = new_node(NODE_FUNC_CALL);
    n->func_name = strdup(current.value);
    advance_token(); // consume function name

    expect(TOKEN_LPAREN, "'('");

    // Parse one argument
    if (current.type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Expected argument identifier\n");
        exit(1);
    }

    ASTNode *arg = new_node(NODE_VAR);
    arg->set_name = strdup(current.value);
    advance_token();

    n->arg_count = 1;
    n->args = calloc(1, sizeof(ASTNode *));
    n->args[0] = arg;

    expect(TOKEN_RPAREN, "')'");
    return n;
}

ASTNode *parse_object_literal()
{
    expect(TOKEN_LBRACE, "'{'");

    int cap = 4, count = 0;
    KeyValuePair *pairs = malloc(cap * sizeof(KeyValuePair));

    while (current.type != TOKEN_RBRACE)
    {
        if (count == cap)
        {
            cap *= 2;
            pairs = realloc(pairs, cap * sizeof(KeyValuePair));
        }

        if (current.type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Expected key in object\n");
            exit(1);
        }

        char *key = strdup(current.value);
        advance_token();
        expect(TOKEN_COLON, "':'");

        Value val;

        if (current.type == TOKEN_STRING)
        {
            val.type = VAL_STRING;
            val.str = strdup(current.value);
            advance_token();
        }
        else if (current.type == TOKEN_NUMBER)
        {
            val.type = VAL_NUMBER;
            val.num = atof(current.value);
            advance_token();
        }
        else if (current.type == TOKEN_LBRACE)
        {
            ASTNode *inner_obj_node = parse_object_literal();
            val = inner_obj_node->literal_value;
            free(inner_obj_node);
        }
        else
        {
            fprintf(stderr, "Expected literal value in object\n");
            exit(1);
        }

        pairs[count].key = key;
        pairs[count].value = val;
        count++;

        if (!match(TOKEN_COMMA))
            break;
    }

    expect(TOKEN_RBRACE, "'}'");

    Object *obj = malloc(sizeof(Object));
    obj->count = count;
    obj->capacity = cap;
    obj->pairs = pairs;

    ASTNode *obj_node = new_node(NODE_SET);
    obj_node->literal_value.type = VAL_OBJECT;
    obj_node->literal_value.obj = obj;

    return obj_node;
}

static ASTNode *parse_statement()
{
    if (match(TOKEN_SET))
        return parse_set_stmt();
    if (current.type == TOKEN_IDENTIFIER)
        return parse_func_call();

    fprintf(stderr, "Parse error: unexpected token '%s'\n", current.value);
    exit(1);
}

/* --- public API --- */
ASTNode **parse_program(Lexer *lexer, int *out_count)
{
    L = lexer;
    advance_token();

    int cap = 8, count = 0;
    ASTNode **list = malloc(sizeof(ASTNode *) * cap);

    while (current.type != TOKEN_EOF)
    {
        if (current.type == TOKEN_NEWLINE)
        {
            advance_token();
            continue;
        }
        if (count == cap)
        {
            cap *= 2;
            list = realloc(list, cap * sizeof(ASTNode *));
        }
        list[count++] = parse_statement();
    }

    *out_count = count;
    return list;
}
