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