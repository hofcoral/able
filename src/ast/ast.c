#include <stdlib.h>

#include "ast/ast.h"

ASTNode *new_node(NodeType type, int line, int column)
{
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = type;
    n->line = line;
    n->column = column;
    return n;
}

void add_child(ASTNode *parent, ASTNode *child)
{
    parent->children = realloc(parent->children, sizeof(ASTNode *) * (parent->child_count + 1));
    parent->children[parent->child_count++] = child;
}

ASTNode *new_set_node(char *name, ASTNode *attr, int line, int column)
{
    ASTNode *n = new_node(NODE_SET, line, column);
    n->data.set.set_name = name;
    n->data.set.set_attr = attr;
    return n;
}

ASTNode *new_var_node(char *name, int line, int column)
{
    ASTNode *n = new_node(NODE_VAR, line, column);
    n->data.var.var_name = name;
    return n;
}

ASTNode *new_attr_node(char *object, char *attr, int line, int column)
{
    ASTNode *n = new_node(NODE_ATTR_ACCESS, line, column);
    n->data.attr.object_name = object;
    n->data.attr.attr_name = attr;
    return n;
}

ASTNode *new_func_call_node(ASTNode *callee, char *func_name, int line, int column)
{
    ASTNode *n = new_node(NODE_FUNC_CALL, line, column);
    n->data.call.func_callee = callee;
    n->data.call.func_name = func_name;
    return n;
}

ASTNode *new_literal_node(Value value, int line, int column)
{
    ASTNode *n = new_node(NODE_LITERAL, line, column);
    n->data.literal = value;
    return n;
}

ASTNode *new_binary_node(BinaryOp op, int line, int column)
{
    ASTNode *n = new_node(NODE_BINARY, line, column);
    n->data.binary.op = op;
    return n;
}

static void free_node(ASTNode *n)
{
    if (!n)
        return;

    switch (n->type)
    {
    case NODE_SET:
        free(n->data.set.set_name);
        if (n->data.set.set_attr)
            free_node(n->data.set.set_attr);
        break;
    case NODE_VAR:
        free(n->data.var.var_name);
        break;
    case NODE_FUNC_CALL:
        free(n->data.call.func_name);
        if (n->data.call.func_callee)
            free_node(n->data.call.func_callee);
        break;
    case NODE_ATTR_ACCESS:
        free(n->data.attr.object_name);
        free(n->data.attr.attr_name);
        break;
    case NODE_LITERAL:
        free_value(n->data.literal);
        break;
    default:
        break;
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