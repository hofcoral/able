#include <stdlib.h>

#include "ast/ast.h"

static void free_node(ASTNode *n)
{
    if (!n)
        return;

    if (n->type == NODE_SET)
    {
        free(n->set_name);
        if (n->is_copy)
        {
            free(n->copy_from_var);
        }
        else
        {
            free_value(n->literal_value); // deep free for VAL_OBJECT
        }
    }
    else if (n->type == NODE_FUNC_CALL)
    {
        free(n->func_name);
        for (int j = 0; j < n->arg_count; j++)
            free_node(n->args[j]);
        free(n->args);
    }

    free(n);
}

void free_ast(ASTNode **nodes, int count)
{
    for (int i = 0; i < count; i++)
    {
        free_node(nodes[i]);
    }

    free(nodes);
}
