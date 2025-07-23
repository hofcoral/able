#include <stdlib.h>

#include "ast/ast.h"

ASTNode *new_node(NodeType type)
{
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type = type;
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
        free(n->set_name);
        if (n->set_attr)
            free_node(n->set_attr);

        if (n->is_copy)
        {
            if (n->copy_from_attr)
                free_node(n->copy_from_attr);
            else
                free(n->copy_from_var);
        }
        else
        {
            free_value(n->literal_value);
        }
    }

    if (n->type == NODE_FUNC_CALL)
    {
        free(n->func_name);
    }

    if (n->type == NODE_ATTR_ACCESS)
    {
        free(n->object_name);
        free(n->attr_name);
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