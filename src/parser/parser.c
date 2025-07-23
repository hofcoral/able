#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser/parser.h"
#include "types/value.h"
#include "lexer/lexer.h"
#include "types/object.h"
#include "types/function.h"
#include "ast/ast.h"
#include "utils/utils.h"

/* --- helpers --- */
static Token current;
static Lexer *L;

static ASTNode *parse_statement();
static ASTNode *parse_return_stmt();

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
        log_error("Parse error: expected %s", msg);
        exit(1);
    }
}

/* --- parsing functions --- */
static ASTNode *parse_identifier_chain()
{
    if (current.type != TOKEN_IDENTIFIER)
    {
        log_error("Expected identifier");
        exit(1);
    }

    char *first = strdup(current.value);
    advance_token();

    if (!match(TOKEN_DOT))
    {
        ASTNode *var = new_node(NODE_VAR);
        var->set_name = first;
        return var;
    }

    ASTNode *base = new_node(NODE_ATTR_ACCESS);
    base->object_name = first;
    base->child_count = 0;
    base->children = NULL;

    do
    {
        if (current.type != TOKEN_IDENTIFIER)
        {
            log_error("Expected attribute name after '.'");
            exit(1);
        }

        ASTNode *attr = new_node(NODE_ATTR_ACCESS);
        attr->attr_name = strdup(current.value);
        attr->child_count = 0;
        attr->children = NULL;
        advance_token();

        add_child(base, attr);
    } while (match(TOKEN_DOT));

    return base;
}

static ASTNode *parse_literal_node()
{
    ASTNode *n = new_node(NODE_LITERAL);

    if (current.type == TOKEN_STRING)
    {
        n->literal_value.type = VAL_STRING;
        n->literal_value.str = strdup(current.value);
        advance_token();
    }
    else if (current.type == TOKEN_NUMBER)
    {
        n->literal_value.type = VAL_NUMBER;
        n->literal_value.num = atof(current.value);
        advance_token();
    }
    else if (current.type == TOKEN_LBRACE)
    {
        ASTNode *obj = parse_object_literal();
        n->literal_value = obj->literal_value;
        free(obj);
    }
    else
    {
        log_error("Expected literal value");
        exit(1);
    }

    return n;
}

static Function *parse_function_def()
{
    expect(TOKEN_LPAREN, "'('");

    int cap = 4, count = 0;
    char **params = malloc(sizeof(char *) * cap);

    if (current.type != TOKEN_RPAREN)
    {
        while (1)
        {
            if (current.type != TOKEN_IDENTIFIER)
            {
                log_error("Expected parameter name");
                exit(1);
            }

            if (count == cap)
            {
                cap *= 2;
                params = realloc(params, sizeof(char *) * cap);
            }

            params[count++] = strdup(current.value);
            advance_token();

            if (!match(TOKEN_COMMA))
                break;
        }
    }

    expect(TOKEN_RPAREN, "')'");
    expect(TOKEN_COLON, "':'");

    if (current.type == TOKEN_NEWLINE)
        advance_token();

    ASTNode **body = malloc(sizeof(ASTNode *));
    body[0] = parse_statement();

    Function *fn = malloc(sizeof(Function));
    fn->param_count = count;
    fn->params = params;
    fn->body = body;
    fn->body_count = 1;
    return fn;
}

static void parse_literal_into_set(ASTNode *n)
{
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
        ASTNode *src = parse_identifier_chain();
        n->is_copy = true;
        if (src->type == NODE_VAR)
        {
            n->copy_from_var = src->set_name;
            free(src);
        }
        else
        {
            n->copy_from_attr = src;
        }
    }
    else
    {
        log_error("Current: %s", current.value);
        log_error("Expected string, number, object, or identifier after 'to'");
        exit(1);
    }
}

static ASTNode *parse_set_stmt()
{
    ASTNode *n = new_node(NODE_SET);

    ASTNode *dest = parse_identifier_chain();
    if (dest->type == NODE_VAR)
    {
        n->set_name = dest->set_name;
        free(dest);
    }
    else
    {
        n->set_attr = dest;
    }

    if (current.type != TOKEN_TO)
    {
        log_error("Expected 'to' after variable name");
        exit(1);
    }
    advance_token();

    if (current.type == TOKEN_LPAREN)
    {
        Function *fn = parse_function_def();
        n->literal_value.type = VAL_FUNCTION;
        n->literal_value.func = fn;
        n->is_copy = false;
    }
    else
    {
        parse_literal_into_set(n);
    }

    return n;
}

static ASTNode *finish_func_call(ASTNode *callee)
{
    ASTNode *n = new_node(NODE_FUNC_CALL);
    n->func_callee = callee;

    if (callee->type == NODE_VAR)
        n->func_name = strdup(callee->set_name);
    else
        n->func_name = NULL;

    expect(TOKEN_LPAREN, "'('");

    n->children = NULL;
    n->child_count = 0;

    if (current.type != TOKEN_RPAREN)
    {
        while (1)
        {
            ASTNode *arg = parse_argument();
            add_child(n, arg);

            if (!match(TOKEN_COMMA))
                break;
        }
    }

    expect(TOKEN_RPAREN, "')'");
    return n;
}

static ASTNode *parse_func_call()
{
    ASTNode *callee = parse_identifier_chain();
    return finish_func_call(callee);
}

ASTNode *parse_argument()
{
    if (current.type == TOKEN_IDENTIFIER)
    {
        ASTNode *node = parse_identifier_chain();
        if (current.type == TOKEN_LPAREN)
        {
            return finish_func_call(node);
        }
        return node;
    }
    else if (current.type == TOKEN_STRING || current.type == TOKEN_NUMBER || current.type == TOKEN_LBRACE)
    {
        return parse_literal_node();
    }

    log_error("Invalid argument");
    exit(1);
}

ASTNode *parse_object_literal()
{
    expect(TOKEN_LBRACE, "'{'");

    int cap = 4, count = 0;
    KeyValuePair *pairs = malloc(cap * sizeof(KeyValuePair));

    while (current.type != TOKEN_RBRACE)
    {
        // Skip newlines
        while (current.type == TOKEN_NEWLINE)
            advance_token();

        if (count == cap)
        {
            cap *= 2;
            pairs = realloc(pairs, cap * sizeof(KeyValuePair));
        }

        if (current.type != TOKEN_IDENTIFIER)
        {
            log_error("Expected key in object");
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
            log_error("Expected literal value in object");
            exit(1);
        }

        pairs[count].key = key;
        pairs[count].value = val;
        count++;

        if (!match(TOKEN_COMMA))
        {
            // Allow trailing newline(s) after last value
            while (current.type == TOKEN_NEWLINE)
                advance_token();
            break;
        }
    }

    expect(TOKEN_RBRACE, "'}'");

    Object *obj = malloc(sizeof(Object));
    obj->count = count;
    obj->capacity = cap;
    obj->pairs = pairs;

    ASTNode *obj_node = new_node(NODE_LITERAL);
    obj_node->literal_value.type = VAL_OBJECT;
    obj_node->literal_value.obj = obj;

    return obj_node;
}

static ASTNode *parse_return_stmt()
{
    ASTNode *n = new_node(NODE_RETURN);
    ASTNode *expr = parse_argument();
    add_child(n, expr);
    return n;
}

static ASTNode *parse_statement()
{
    if (match(TOKEN_SET))
        return parse_set_stmt();
    if (match(TOKEN_RETURN))
        return parse_return_stmt();
    if (current.type == TOKEN_IDENTIFIER)
        return parse_func_call();

    log_error("Parse error: unexpected token '%s'", current.value);
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
