#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser/parser.h"
#include "types/value.h"
#include "lexer/lexer.h"
#include "types/object.h"
#include "types/function.h"
#include "types/list.h"
#include "ast/ast.h"
#include "utils/utils.h"

/* --- helpers --- */
static Token current;
static int prev_line;
static int prev_col;
static Lexer *L;

static ASTNode *parse_statement();
static ASTNode *parse_return_stmt();
static ASTNode *finish_func_call(ASTNode *callee);
static ASTNode *parse_func_call();
static ASTNode *parse_expression();
static ASTNode *parse_arithmetic();
static ASTNode *parse_block();
static ASTNode *parse_if_stmt();
static ASTNode *parse_list_literal();
ASTNode *parse_argument();

static void advance_token() {
    prev_line = current.line;
    prev_col = current.column;
    current = next_token(L);
}

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
        log_script_error(current.line, current.column, "Parse error: expected %s", msg);
        exit(1);
    }
}

/* --- parsing functions --- */
static ASTNode *parse_identifier_chain()
{
    if (current.type != TOKEN_IDENTIFIER)
    {
        log_script_error(current.line, current.column, "Expected identifier");
        exit(1);
    }

    char *first = strdup(current.value);
    int id_line = current.line;
    int id_col = current.column;
    advance_token();

    if (!match(TOKEN_DOT))
    {
        return new_var_node(first, id_line, id_col);
    }

    ASTNode *base = new_attr_access_node(first, NULL, id_line, id_col);

    do
    {
        if (current.type != TOKEN_IDENTIFIER)
        {
            log_script_error(current.line, current.column, "Expected attribute name after '.'");
            exit(1);
        }

        ASTNode *attr = new_attr_access_node(NULL, strdup(current.value),
                                            current.line, current.column);
        advance_token();

        add_child(base, attr);
    } while (match(TOKEN_DOT));

    return base;
}

static ASTNode *parse_literal_node()
{
    ASTNode *n = new_node(NODE_LITERAL, current.line, current.column);

    if (current.type == TOKEN_STRING)
    {
        n->data.lit.literal_value.type = VAL_STRING;
        n->data.lit.literal_value.str = strdup(current.value);
        advance_token();
    }
    else if (current.type == TOKEN_NUMBER)
    {
        n->data.lit.literal_value.type = VAL_NUMBER;
        n->data.lit.literal_value.num = atof(current.value);
        advance_token();
    }
    else if (current.type == TOKEN_TRUE || current.type == TOKEN_FALSE)
    {
        n->data.lit.literal_value.type = VAL_BOOL;
        n->data.lit.literal_value.boolean = (current.type == TOKEN_TRUE);
        advance_token();
    }
    else if (current.type == TOKEN_NULL)
    {
        n->data.lit.literal_value.type = VAL_NULL;
        advance_token();
    }
    else if (current.type == TOKEN_LBRACE)
    {
        ASTNode *obj = parse_object_literal();
        n->data.lit.literal_value = obj->data.lit.literal_value;
        free(obj);
    }
    else if (current.type == TOKEN_LBRACKET)
    {
        ASTNode *lst = parse_list_literal();
        n->data.lit.literal_value = lst->data.lit.literal_value;
        free(lst);
    }
    else
    {
        log_script_error(current.line, current.column, "Expected literal value");
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
                log_script_error(current.line, current.column, "Expected parameter name");
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

    int body_cap = 4, body_count = 0;
    ASTNode **body = malloc(sizeof(ASTNode *) * body_cap);

    if (match(TOKEN_NEWLINE))
    {
        expect(TOKEN_INDENT, "indent");

        while (current.type != TOKEN_DEDENT && current.type != TOKEN_EOF)
        {
            if (current.type == TOKEN_NEWLINE || current.type == TOKEN_INDENT)
            {
                advance_token();
                continue;
            }

            if (body_count == body_cap)
            {
                body_cap *= 2;
                body = realloc(body, sizeof(ASTNode *) * body_cap);
            }

            body[body_count++] = parse_statement();
        }

        expect(TOKEN_DEDENT, "dedent");
    }
    else
    {
        body[body_count++] = parse_statement();
    }

    Function *fn = malloc(sizeof(Function));
    fn->name = NULL;
    fn->param_count = count;
    fn->params = params;
    fn->body = body;
    fn->body_count = body_count;
    fn->env = NULL;
    return fn;
}


static ASTNode *parse_set_stmt()
{
    int line = prev_line;
    int col = prev_col;
    ASTNode *dest = parse_identifier_chain();
    char *set_name = NULL;
    ASTNode *set_attr = NULL;
    if (dest->type == NODE_VAR)
    {
        set_name = dest->data.set.set_name;
        free(dest);
    }
    else
    {
        set_attr = dest;
    }
    ASTNode *n = new_set_node(set_name, set_attr, line, col);

    if (current.type != TOKEN_TO)
    {
        log_script_error(current.line, current.column, "Expected 'to' after variable name");
        exit(1);
    }
    advance_token();

    if (current.type == TOKEN_LPAREN)
    {
        // Determine if this is a function definition by peeking for ':' after ')'
        size_t save_pos = L->pos;
        int is_func = 0;
        int depth = 1;
        while (save_pos < L->length && depth > 0)
        {
            char c = L->source[save_pos++];
            if (c == '(')
                depth++;
            else if (c == ')')
                depth--;
        }
        if (depth == 0 && save_pos < L->length && L->source[save_pos] == ':')
            is_func = 1;

        if (is_func)
        {
            Function *fn = parse_function_def();
            fn->name = strdup(n->data.set.set_name);
            ASTNode *lit = new_node(NODE_LITERAL, line, col);
            lit->data.lit.literal_value.type = VAL_FUNCTION;
            lit->data.lit.literal_value.func = fn;
            add_child(n, lit);
            return n;
        }
    }

    ASTNode *expr = parse_expression();
    add_child(n, expr);

    return n;
}

static ASTNode *finish_func_call(ASTNode *callee)
{
    ASTNode *n = new_func_call_node(callee);

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

static ASTNode *parse_primary();
static ASTNode *parse_expression();

static ASTNode *parse_unary()
{
    if (match(TOKEN_MINUS))
    {
        ASTNode *right = parse_unary();
        ASTNode *zero = new_node(NODE_LITERAL, prev_line, prev_col);
        zero->data.lit.literal_value.type = VAL_NUMBER;
        zero->data.lit.literal_value.num = 0;
        ASTNode *n = new_node(NODE_BINARY, prev_line, prev_col);
        n->data.binary.op = OP_SUB;
        add_child(n, zero);
        add_child(n, right);
        return n;
    }
    return parse_primary();
}

static ASTNode *parse_factor()
{
    ASTNode *node = parse_unary();
    while (current.type == TOKEN_STAR || current.type == TOKEN_SLASH || current.type == TOKEN_PERCENT)
    {
        BinaryOp op;
        if (current.type == TOKEN_STAR)
            op = OP_MUL;
        else if (current.type == TOKEN_SLASH)
            op = OP_DIV;
        else
            op = OP_MOD;
        advance_token();
        ASTNode *right = parse_unary();
        ASTNode *bin = new_node(NODE_BINARY, prev_line, prev_col);
        bin->data.binary.op = op;
        add_child(bin, node);
        add_child(bin, right);
        node = bin;
    }
    return node;
}

static ASTNode *parse_arithmetic()
{
    ASTNode *node = parse_factor();
    while (current.type == TOKEN_PLUS || current.type == TOKEN_MINUS)
    {
        BinaryOp op = current.type == TOKEN_PLUS ? OP_ADD : OP_SUB;
        advance_token();
        ASTNode *right = parse_factor();
        ASTNode *bin = new_node(NODE_BINARY, prev_line, prev_col);
        bin->data.binary.op = op;
        add_child(bin, node);
        add_child(bin, right);
        node = bin;
    }
    return node;
}

static ASTNode *parse_expression()
{
    ASTNode *node = parse_arithmetic();
    while (current.type == TOKEN_EQ || current.type == TOKEN_STRICT_EQ ||
           current.type == TOKEN_LT || current.type == TOKEN_GT ||
           current.type == TOKEN_LTE || current.type == TOKEN_GTE)
    {
        BinaryOp op;
        switch (current.type)
        {
        case TOKEN_EQ:
            op = OP_EQ;
            break;
        case TOKEN_STRICT_EQ:
            op = OP_STRICT_EQ;
            break;
        case TOKEN_LT:
            op = OP_LT;
            break;
        case TOKEN_GT:
            op = OP_GT;
            break;
        case TOKEN_LTE:
            op = OP_LTE;
            break;
        default:
            op = OP_GTE;
        }
        advance_token();
        ASTNode *right = parse_arithmetic();
        ASTNode *bin = new_node(NODE_BINARY, prev_line, prev_col);
        bin->data.binary.op = op;
        add_child(bin, node);
        add_child(bin, right);
        node = bin;
    }
    return node;
}

static ASTNode *parse_primary()
{
    if (current.type == TOKEN_IDENTIFIER)
    {
        ASTNode *node = parse_identifier_chain();
        if (current.type == TOKEN_LPAREN)
            return finish_func_call(node);
        return node;
    }
    else if (current.type == TOKEN_STRING || current.type == TOKEN_NUMBER ||
             current.type == TOKEN_TRUE || current.type == TOKEN_FALSE ||
             current.type == TOKEN_NULL || current.type == TOKEN_LBRACE ||
             current.type == TOKEN_LBRACKET)
    {
        return parse_literal_node();
    }
    else if (match(TOKEN_LPAREN))
    {
        ASTNode *expr = parse_expression();
        expect(TOKEN_RPAREN, ")");
        return expr;
    }

    log_script_error(current.line, current.column, "Invalid expression");
    exit(1);
}

ASTNode *parse_argument()
{
    return parse_expression();
}

ASTNode *parse_object_literal()
{
    expect(TOKEN_LBRACE, "'{'");
    int line = prev_line;
    int col = prev_col;

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
            log_script_error(current.line, current.column, "Expected key in object");
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
        else if (current.type == TOKEN_TRUE || current.type == TOKEN_FALSE)
        {
            val.type = VAL_BOOL;
            val.boolean = (current.type == TOKEN_TRUE);
            advance_token();
        }
        else if (current.type == TOKEN_NULL)
        {
            val.type = VAL_NULL;
            advance_token();
        }
        else if (current.type == TOKEN_LBRACE)
        {
            ASTNode *inner_obj_node = parse_object_literal();
            val = inner_obj_node->data.lit.literal_value;
            free(inner_obj_node);
        }
        else
        {
            log_script_error(current.line, current.column, "Expected literal value in object");
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

    ASTNode *obj_node = new_node(NODE_LITERAL, line, col);
    obj_node->data.lit.literal_value.type = VAL_OBJECT;
    obj_node->data.lit.literal_value.obj = obj;

    return obj_node;
}

ASTNode *parse_list_literal()
{
    expect(TOKEN_LBRACKET, "'['");
    int line = prev_line;
    int col = prev_col;

    int cap = 4, count = 0;
    Value *items = malloc(sizeof(Value) * cap);

    while (current.type != TOKEN_RBRACKET)
    {
        while (current.type == TOKEN_NEWLINE)
            advance_token();

        if (count == cap)
        {
            cap *= 2;
            items = realloc(items, sizeof(Value) * cap);
        }

        if (current.type == TOKEN_STRING)
        {
            items[count].type = VAL_STRING;
            items[count].str = strdup(current.value);
            advance_token();
        }
        else if (current.type == TOKEN_NUMBER)
        {
            items[count].type = VAL_NUMBER;
            items[count].num = atof(current.value);
            advance_token();
        }
        else if (current.type == TOKEN_TRUE || current.type == TOKEN_FALSE)
        {
            items[count].type = VAL_BOOL;
            items[count].boolean = (current.type == TOKEN_TRUE);
            advance_token();
        }
        else if (current.type == TOKEN_NULL)
        {
            items[count].type = VAL_NULL;
            advance_token();
        }
        else if (current.type == TOKEN_LBRACE)
        {
            ASTNode *obj = parse_object_literal();
            items[count] = obj->data.lit.literal_value;
            free(obj);
        }
        else if (current.type == TOKEN_LBRACKET)
        {
            ASTNode *lst = parse_list_literal();
            items[count] = lst->data.lit.literal_value;
            free(lst);
        }
        else
        {
            log_script_error(current.line, current.column, "Expected literal value in list");
            exit(1);
        }

        count++;
        if (!match(TOKEN_COMMA))
        {
            while (current.type == TOKEN_NEWLINE)
                advance_token();
            break;
        }
    }

    expect(TOKEN_RBRACKET, "]");

    List *list = malloc(sizeof(List));
    list->count = count;
    list->capacity = cap;
    list->items = items;

    ASTNode *node = new_node(NODE_LITERAL, line, col);
    node->data.lit.literal_value.type = VAL_LIST;
    node->data.lit.literal_value.list = list;
    return node;
}

static ASTNode *parse_return_stmt()
{
    int line = prev_line;
    int col = prev_col;
    ASTNode *n = new_node(NODE_RETURN, line, col);
    if (current.type == TOKEN_NEWLINE || current.type == TOKEN_DEDENT || current.type == TOKEN_EOF)
    {
        ASTNode *undef = new_node(NODE_LITERAL, line, col);
        undef->data.lit.literal_value.type = VAL_UNDEFINED;
        add_child(n, undef);
    }
    else
    {
        ASTNode *expr = parse_expression();
        add_child(n, expr);
    }
    return n;
}

static ASTNode *parse_block()
{
    int line = prev_line;
    int col = prev_col;
    ASTNode *block = new_node(NODE_BLOCK, line, col);
    if (match(TOKEN_NEWLINE))
    {
        expect(TOKEN_INDENT, "indent");
        while (current.type != TOKEN_DEDENT && current.type != TOKEN_EOF)
        {
            if (current.type == TOKEN_NEWLINE || current.type == TOKEN_INDENT)
            {
                advance_token();
                continue;
            }
            ASTNode *stmt = parse_statement();
            add_child(block, stmt);
        }
        expect(TOKEN_DEDENT, "dedent");
    }
    else
    {
        ASTNode *stmt = parse_statement();
        add_child(block, stmt);
    }
    return block;
}

static ASTNode *parse_if_stmt()
{
    ASTNode *node = new_node(NODE_IF, prev_line, prev_col);
    ASTNode *cond = parse_expression();
    expect(TOKEN_COLON, "':'");
    ASTNode *then_block = parse_block();
    add_child(node, cond);
    add_child(node, then_block);

    if (match(TOKEN_ELIF))
    {
        ASTNode *elif_node = parse_if_stmt();
        add_child(node, elif_node);
    }
    else if (match(TOKEN_ELSE))
    {
        expect(TOKEN_COLON, "':'");
        ASTNode *else_block = parse_block();
        add_child(node, else_block);
    }
    return node;
}

static ASTNode *parse_statement()
{
    while (current.type == TOKEN_NEWLINE)
        advance_token();
    if (match(TOKEN_SET))
        return parse_set_stmt();
    if (match(TOKEN_RETURN))
        return parse_return_stmt();
    if (match(TOKEN_IF))
        return parse_if_stmt();
    if (current.type == TOKEN_IDENTIFIER)
        return parse_func_call();

    log_script_error(current.line, current.column, "Parse error: unexpected token '%s'", current.value);
    exit(1);
}

/* --- public API --- */
ASTNode **parse_program(Lexer *lexer, int *out_count)
{
    L = lexer;
    current = next_token(L);
    prev_line = current.line;
    prev_col = current.column;

    int cap = 8, count = 0;
    ASTNode **list = malloc(sizeof(ASTNode *) * cap);

    while (current.type != TOKEN_EOF)
    {
        if (current.type == TOKEN_NEWLINE || current.type == TOKEN_INDENT || current.type == TOKEN_DEDENT)
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
