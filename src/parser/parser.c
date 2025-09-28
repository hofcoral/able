#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser/parser.h"
#include "types/value.h"
#include "lexer/lexer.h"
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
static ASTNode *parse_expression();
static ASTNode *parse_ternary();
static ASTNode *parse_logical();
static ASTNode *parse_comparison();
static ASTNode *parse_arithmetic();
static ASTNode *parse_block();
static ASTNode *parse_if_stmt();
static ASTNode *parse_for_stmt();
static ASTNode *parse_while_stmt();
static ASTNode *parse_break_stmt();
static ASTNode *parse_continue_stmt();
static ASTNode *parse_import_module_stmt();
static ASTNode *parse_from_import_stmt();
static ASTNode *parse_list_literal();
static ASTNode *parse_method_def(char *name, bool is_static, int line, int col);
static ASTNode *parse_class_def();
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
        free(n);
        return parse_object_literal();
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

static void parse_function_parts(char ***out_params, int *out_param_count,
                                 ASTNode ***out_body, int *out_body_count)
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

    *out_params = params;
    *out_param_count = count;
    *out_body = body;
    *out_body_count = body_count;
}

static Function *build_function(char **params, int param_count, ASTNode **body, int body_count)
{
    Function *fn = malloc(sizeof(Function));
    fn->name = NULL;
    fn->param_count = param_count;
    fn->params = params;
    fn->body = body;
    fn->body_count = body_count;
    fn->env = NULL;
    fn->bind_on_access = false;
    return fn;
}

static ASTNode *parse_function_literal_node(const char *name_hint, int line, int col)
{
    char **params;
    int param_count;
    ASTNode **body;
    int body_count;
    parse_function_parts(&params, &param_count, &body, &body_count);

    Function *fn = build_function(params, param_count, body, body_count);
    if (name_hint)
        fn->name = strdup(name_hint);

    ASTNode *lit = new_node(NODE_LITERAL, line, col);
    lit->data.lit.literal_value.type = VAL_FUNCTION;
    lit->data.lit.literal_value.func = fn;
    return lit;
}

static ASTNode *parse_fun_declaration(bool is_private)
{
    int line = prev_line;
    int col = prev_col;

    if (current.type != TOKEN_IDENTIFIER)
    {
        log_script_error(current.line, current.column, "Expected function name");
        exit(1);
    }

    char *name = strdup(current.value);
    advance_token();

    ASTNode *assign = new_set_node(name, NULL, line, col);
    ASTNode *lit = parse_function_literal_node(name, line, col);
    add_child(assign, lit);

    if (is_private)
        assign->is_private = true;

    return assign;
}

static ASTNode *parse_assignment(ASTNode *dest)
{
    int line = dest->line;
    int col = dest->column;

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

    ASTNode *assign = new_set_node(set_name, set_attr, line, col);
    ASTNode *expr = parse_expression();
    add_child(assign, expr);
    return assign;
}
static ASTNode *parse_class_def()
{
    int line = prev_line;
    int col = prev_col;
    if (current.type != TOKEN_IDENTIFIER)
    {
        log_script_error(current.line, current.column, "Expected class name");
        exit(1);
    }
    char *name = strdup(current.value);
    advance_token();

    expect(TOKEN_LPAREN, "'('");

    int cap = 4, count = 0;
    char **bases = malloc(sizeof(char *) * cap);
    if (current.type != TOKEN_RPAREN)
    {
        while (1)
        {
            if (current.type != TOKEN_IDENTIFIER)
            {
                log_script_error(current.line, current.column, "Expected base name");
                exit(1);
            }
            if (count == cap)
            {
                cap *= 2;
                bases = realloc(bases, sizeof(char *) * cap);
            }
            bases[count++] = strdup(current.value);
            advance_token();
            if (!match(TOKEN_COMMA))
                break;
        }
    }
    expect(TOKEN_RPAREN, ")");
    expect(TOKEN_COLON, ":");

    ASTNode *cls = new_node(NODE_CLASS_DEF, line, col);
    cls->data.cls.class_name = name;
    cls->data.cls.base_names = bases;
    cls->data.cls.base_count = count;

    if (!match(TOKEN_NEWLINE))
    {
        log_script_error(current.line, current.column, "Expected newline after class header");
        exit(1);
    }
    expect(TOKEN_INDENT, "indent");
    bool static_flag = false;
    while (current.type != TOKEN_DEDENT && current.type != TOKEN_EOF)
    {
        if (current.type == TOKEN_NEWLINE || current.type == TOKEN_INDENT)
        {
            advance_token();
            continue;
        }
        if (match(TOKEN_AT_STATIC))
        {
            static_flag = true;
            continue;
        }
        if (match(TOKEN_FUN))
        {
            if (current.type != TOKEN_IDENTIFIER)
            {
                log_script_error(current.line, current.column, "Expected method name");
                exit(1);
            }
            char *mname = strdup(current.value);
            advance_token();
            ASTNode *m = parse_method_def(mname, static_flag, prev_line, prev_col);
            add_child(cls, m);
            static_flag = false;
            continue;
        }

        log_script_error(current.line, current.column, "Unexpected token in class body");
        exit(1);
    }
    expect(TOKEN_DEDENT, "dedent");
    return cls;
}

static ASTNode *parse_method_def(char *name, bool is_static, int line, int col)
{
    char **params;
    int param_count;
    ASTNode **body;
    int body_count;
    parse_function_parts(&params, &param_count, &body, &body_count);

    ASTNode *m = new_node(NODE_METHOD_DEF, line, col);
    m->data.method.method_name = name;
    m->data.method.params = params;
    m->data.method.param_count = param_count;
    m->children = body;
    m->child_count = body_count;
    m->is_static = is_static;
    return m;
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

static ASTNode *parse_primary();
static ASTNode *parse_expression();
static ASTNode *parse_postfix();

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
    if (match(TOKEN_NOT))
    {
        ASTNode *expr = parse_unary();
        return new_unary_node(UNARY_NOT, expr, prev_line, prev_col);
    }
    return parse_postfix();
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

static ASTNode *parse_comparison()
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

static ASTNode *parse_logical()
{
    ASTNode *node = parse_comparison();
    while (current.type == TOKEN_AND || current.type == TOKEN_OR)
    {
        BinaryOp op = current.type == TOKEN_AND ? OP_AND : OP_OR;
        advance_token();
        ASTNode *right = parse_comparison();
        ASTNode *bin = new_node(NODE_BINARY, prev_line, prev_col);
        bin->data.binary.op = op;
        add_child(bin, node);
        add_child(bin, right);
        node = bin;
    }
    return node;
}

static ASTNode *parse_ternary()
{
    ASTNode *condition = parse_logical();
    if (match(TOKEN_QUESTION))
    {
        ASTNode *true_expr = parse_ternary();
        expect(TOKEN_COLON, "':'");
        ASTNode *false_expr = parse_ternary();
        ASTNode *tern = new_ternary_node(condition, true_expr, false_expr,
                                         prev_line, prev_col);
        return tern;
    }
    return condition;
}

static ASTNode *parse_expression()
{
    return parse_ternary();
}

static ASTNode *parse_primary()
{
    if (match(TOKEN_FUN))
        return parse_function_literal_node(NULL, prev_line, prev_col);
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

static ASTNode *parse_postfix()
{
    ASTNode *node = parse_primary();
    while (1)
    {
        if (match(TOKEN_INC))
        {
            if (node->type != NODE_VAR && node->type != NODE_ATTR_ACCESS)
            {
                log_script_error(prev_line, prev_col, "Invalid increment target");
                exit(1);
            }
            node = new_postfix_inc_node(node);
            continue;
        }
        if (current.type == TOKEN_LBRACKET)
        {
            expect(TOKEN_LBRACKET, "'['");
            int line = prev_line;
            int col = prev_col;
            bool is_slice = false;
            bool has_start = false;
            bool has_end = false;
            ASTNode *start = NULL;
            ASTNode *end = NULL;

            if (current.type != TOKEN_COLON && current.type != TOKEN_RBRACKET)
            {
                start = parse_expression();
                has_start = true;
            }
            if (match(TOKEN_COLON))
            {
                is_slice = true;
                if (current.type != TOKEN_RBRACKET)
                {
                    end = parse_expression();
                    has_end = true;
                }
            }
            else if (!has_start)
            {
                log_script_error(current.line, current.column, "Expected index expression");
                exit(1);
            }

            expect(TOKEN_RBRACKET, "]");
            ASTNode *idx = new_index_node(is_slice, has_start, has_end, line, col);
            add_child(idx, node);
            if (has_start)
                add_child(idx, start);
            if (has_end)
                add_child(idx, end);
            node = idx;
            continue;
        }
        if (current.type == TOKEN_LPAREN)
        {
            node = finish_func_call(node);
            continue;
        }
        break;
    }
    return node;
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
    char **keys = malloc(sizeof(char *) * cap);
    ASTNode **vals = malloc(sizeof(ASTNode *) * cap);

    while (current.type != TOKEN_RBRACE)
    {
        while (current.type == TOKEN_NEWLINE)
            advance_token();

        if (count == cap)
        {
            cap *= 2;
            keys = realloc(keys, sizeof(char *) * cap);
            vals = realloc(vals, sizeof(ASTNode *) * cap);
        }

        if (current.type != TOKEN_IDENTIFIER)
        {
            log_script_error(current.line, current.column, "Expected key in object");
            exit(1);
        }

        char *key = strdup(current.value);
        int key_line = current.line;
        int key_col = current.column;
        advance_token();

        ASTNode *val_node;
        if (match(TOKEN_COLON))
        {
            val_node = parse_expression();
        }
        else
        {
            val_node = new_var_node(strdup(key), key_line, key_col);
        }

        keys[count] = key;
        vals[count] = val_node;
        count++;

        if (!match(TOKEN_COMMA))
        {
            while (current.type == TOKEN_NEWLINE)
                advance_token();
            break;
        }
    }

    expect(TOKEN_RBRACE, "'}'");

    ASTNode *obj_node = new_node(NODE_OBJECT_LITERAL, line, col);
    obj_node->data.object.keys = keys;
    obj_node->data.object.values = vals;
    obj_node->data.object.pair_count = count;

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

static ASTNode *parse_for_stmt()
{
    int line = prev_line;
    int col = prev_col;
    if (current.type != TOKEN_IDENTIFIER)
    {
        log_script_error(current.line, current.column, "Expected loop variable");
        exit(1);
    }
    char *var = strdup(current.value);
    advance_token();
    expect(TOKEN_OF, "of");
    ASTNode *iter = parse_expression();
    expect(TOKEN_COLON, ":");
    ASTNode *body = parse_block();
    ASTNode *node = new_node(NODE_FOR, line, col);
    node->data.loop.loop_var = var;
    add_child(node, iter);
    add_child(node, body);
    return node;
}

static ASTNode *parse_while_stmt()
{
    int line = prev_line;
    int col = prev_col;
    ASTNode *node = new_node(NODE_WHILE, line, col);
    ASTNode *cond = parse_expression();
    expect(TOKEN_COLON, ":");
    ASTNode *body = parse_block();
    add_child(node, cond);
    add_child(node, body);
    return node;
}

static ASTNode *parse_break_stmt()
{
    return new_node(NODE_BREAK, prev_line, prev_col);
}

static ASTNode *parse_continue_stmt()
{
    return new_node(NODE_CONTINUE, prev_line, prev_col);
}

static char *parse_module_name()
{
    if (current.type == TOKEN_STRING)
    {
        char *name = strdup(current.value);
        advance_token();
        return name;
    }

    if (current.type != TOKEN_IDENTIFIER)
    {
        log_script_error(current.line, current.column, "Expected module name");
        exit(1);
    }

    char *name = strdup(current.value);
    advance_token();
    while (match(TOKEN_DOT))
    {
        if (current.type != TOKEN_IDENTIFIER)
        {
            log_script_error(current.line, current.column,
                             "Expected identifier after '.'");
            exit(1);
        }
        size_t len = strlen(name) + strlen(current.value) + 2;
        char *tmp = malloc(len);
        snprintf(tmp, len, "%s/%s", name, current.value);
        free(name);
        name = tmp;
        advance_token();
    }
    return name;
}

static ASTNode *parse_import_module_stmt()
{
    int line = prev_line;
    int col = prev_col;
    char *name = parse_module_name();
    return new_import_module_node(name, line, col);
}

static ASTNode *parse_from_import_stmt()
{
    int line = prev_line;
    int col = prev_col;
    char *module = parse_module_name();
    expect(TOKEN_IMPORT, "import");
    int cap = 4, count = 0;
    char **names = malloc(sizeof(char *) * cap);
    while (1)
    {
        if (current.type != TOKEN_IDENTIFIER)
        {
            log_script_error(current.line, current.column, "Expected identifier");
            exit(1);
        }
        if (count == cap)
        {
            cap *= 2;
            names = realloc(names, sizeof(char *) * cap);
        }
        names[count++] = strdup(current.value);
        advance_token();
        if (!match(TOKEN_COMMA))
            break;
    }
    return new_import_names_node(module, names, count, line, col);
}

static ASTNode *parse_statement()
{
    while (current.type == TOKEN_NEWLINE)
        advance_token();
    bool private_flag = false;
    if (match(TOKEN_AT_PRIVATE))
    {
        private_flag = true;
        while (current.type == TOKEN_NEWLINE)
            advance_token();
    }
    if (match(TOKEN_FUN))
        return parse_fun_declaration(private_flag);
    if (private_flag && current.type != TOKEN_IDENTIFIER)
    {
        log_script_error(current.line, current.column, "Expected assignment after @private");
        exit(1);
    }
    if (match(TOKEN_RETURN))
        return parse_return_stmt();
    if (match(TOKEN_FOR))
        return parse_for_stmt();
    if (match(TOKEN_WHILE))
        return parse_while_stmt();
    if (match(TOKEN_BREAK))
        return parse_break_stmt();
    if (match(TOKEN_CONTINUE))
        return parse_continue_stmt();
    if (match(TOKEN_IMPORT))
        return parse_import_module_stmt();
    if (match(TOKEN_FROM))
        return parse_from_import_stmt();
    if (match(TOKEN_IF))
        return parse_if_stmt();
    if (match(TOKEN_CLASS))
        return parse_class_def();
    if (current.type == TOKEN_IDENTIFIER)
    {
        ASTNode *id = parse_identifier_chain();
        if (match(TOKEN_ASSIGN))
        {
            ASTNode *n = parse_assignment(id);
            if (private_flag)
                n->is_private = true;
            return n;
        }
        if (private_flag)
        {
            log_script_error(current.line, current.column, "Expected '=' after @private target");
            exit(1);
        }
        if (current.type == TOKEN_LPAREN)
            return finish_func_call(id);
        if (match(TOKEN_INC))
            return new_postfix_inc_node(id);

        log_script_error(current.line, current.column,
                         "Parse error: unexpected token '%s'", current.value);
        exit(1);
    }

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
