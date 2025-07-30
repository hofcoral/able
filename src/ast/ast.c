#include <stdlib.h>
#include <string.h>

#include "ast/ast.h"

ASTNode *new_node(NodeType type, int line, int column)
{
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = type;
    n->line = line;
    n->column = column;
    n->is_static = false;
    n->is_private = false;
    return n;
}

ASTNode *new_var_node(char *name, int line, int column)
{
    ASTNode *n = new_node(NODE_VAR, line, column);
    n->data.set.set_name = name;
    return n;
}

ASTNode *new_attr_access_node(char *object_name, char *attr_name,
                              int line, int column)
{
    ASTNode *n = new_node(NODE_ATTR_ACCESS, line, column);
    n->data.attr.object_name = object_name;
    n->data.attr.attr_name = attr_name;
    return n;
}

ASTNode *new_set_node(char *name, ASTNode *attr, int line, int column)
{
    ASTNode *n = new_node(NODE_SET, line, column);
    n->data.set.set_name = name;
    n->data.set.set_attr = attr;
    return n;
}

ASTNode *new_func_call_node(ASTNode *callee)
{
    ASTNode *n = new_node(NODE_FUNC_CALL, callee->line, callee->column);
    n->data.call.func_callee = callee;
    if (callee->type == NODE_VAR)
        n->data.call.func_name = strdup(callee->data.set.set_name);
    else
        n->data.call.func_name = NULL;
    return n;
}

ASTNode *new_import_module_node(char *module_name, int line, int column)
{
    ASTNode *n = new_node(NODE_IMPORT_MODULE, line, column);
    n->data.import_module.module_name = module_name;
    return n;
}

ASTNode *new_import_names_node(char *module_name, char **names, int name_count,
                               int line, int column)
{
    ASTNode *n = new_node(NODE_IMPORT_NAMES, line, column);
    n->data.import_names.module_name = module_name;
    n->data.import_names.names = names;
    n->data.import_names.name_count = name_count;
    return n;
}

ASTNode *new_postfix_inc_node(ASTNode *target)
{
    ASTNode *n = new_node(NODE_POSTFIX_INC, target->line, target->column);
    add_child(n, target);
    return n;
}

ASTNode *new_unary_node(UnaryOp op, ASTNode *expr, int line, int column)
{
    ASTNode *n = new_node(NODE_UNARY, line, column);
    n->data.unary.op = op;
    add_child(n, expr);
    return n;
}

ASTNode *new_ternary_node(ASTNode *cond, ASTNode *true_expr, ASTNode *false_expr,
                          int line, int column)
{
    ASTNode *n = new_node(NODE_TERNARY, line, column);
    add_child(n, cond);
    add_child(n, true_expr);
    add_child(n, false_expr);
    return n;
}

void add_child(ASTNode *parent, ASTNode *child)
{
    parent->children = realloc(parent->children, sizeof(ASTNode *) * (parent->child_count + 1));
    parent->children[parent->child_count++] = child;
}

static void free_node(ASTNode *n)
{
    if (!n)
        return;

    if (n->type == NODE_SET)
    {
        free(n->data.set.set_name);
        if (n->data.set.set_attr)
            free_node(n->data.set.set_attr);
    }

    if (n->type == NODE_LITERAL)
    {
        free_value(n->data.lit.literal_value);
    }

    if (n->type == NODE_FUNC_CALL)
    {
        free(n->data.call.func_name);
        if (n->data.call.func_callee)
            free_node(n->data.call.func_callee);
    }

    if (n->type == NODE_ATTR_ACCESS)
    {
        free(n->data.attr.object_name);
        free(n->data.attr.attr_name);
    }

    if (n->type == NODE_CLASS_DEF)
    {
        free(n->data.cls.class_name);
        for (int i = 0; i < n->data.cls.base_count; ++i)
            free(n->data.cls.base_names[i]);
        free(n->data.cls.base_names);
    }

    if (n->type == NODE_METHOD_DEF)
    {
        free(n->data.method.method_name);
        for (int i = 0; i < n->data.method.param_count; ++i)
            free(n->data.method.params[i]);
        free(n->data.method.params);
    }

    if (n->type == NODE_FOR)
    {
        free(n->data.loop.loop_var);
    }
    if (n->type == NODE_IMPORT_MODULE)
    {
        free(n->data.import_module.module_name);
    }
    if (n->type == NODE_IMPORT_NAMES)
    {
        free(n->data.import_names.module_name);
        for (int i = 0; i < n->data.import_names.name_count; ++i)
            free(n->data.import_names.names[i]);
        free(n->data.import_names.names);
    }

    // Free any children (used for all types with nested structure)
    for (int i = 0; i < n->child_count; ++i)
    {
        free_node(n->children[i]);
    }

    free(n->children);
    free(n);
}

void free_ast(ASTNode **nodes, int count)
{
    for (int i = 0; i < count; i++)
        free_node(nodes[i]);

    free(nodes);
}