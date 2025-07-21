#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

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

    if (current.type == TOKEN_STRING)
    {
        n->set_value = strdup(current.value);
        advance_token();
    }
    else if (current.type == TOKEN_IDENTIFIER)
    {
        n->copy_from_var = strdup(current.value);
        advance_token();
    }
    else
    {
        fprintf(stderr, "Expected value or variable after 'to'\n");
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

/* --- simple AST print --- */
static void indent(int n)
{
    while (n--)
        putchar(' ');
}

void print_ast(ASTNode **nodes, int count, int ind)
{
    for (int i = 0; i < count; ++i)
    {
        ASTNode *n = nodes[i];
        indent(ind);
        if (n->type == NODE_SET)
        {
            printf("Set(%s = \"%s\")\n", n->set_name, n->set_value);
        }
        else
        {
            printf("Call(%s ...)\n", n->func_name);
            indent(ind + 2);
            puts("Args:");
            for (int a = 0; a < n->arg_count; ++a)
            {
                indent(ind + 4);
                printf("Var(%s)\n", n->args[a]->set_name);
            }
        }
    }
}

/* --- free memory --- */
void free_ast(ASTNode **nodes, int count)
{
    for (int i = 0; i < count; i++)
    {
        ASTNode *n = nodes[i];

        switch (n->type)
        {
        case NODE_SET:
            free(n->set_name);
            if (n->set_value)
                free(n->set_value);
            if (n->copy_from_var)
                free(n->copy_from_var);
            break;

        case NODE_FUNC_CALL:
            free(n->func_name);
            for (int j = 0; j < n->arg_count; j++)
            {
                if (n->args[j])
                {
                    free(n->args[j]); // Directly free the argument node
                }
            }
            free(n->args);
            break;

        case NODE_VAR:
            free(n->set_name);
            break;
        }

        free(n);
    }

    free(nodes);
}
