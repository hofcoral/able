#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/variable.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"
#include "utils/utils.h"

void run_ast(ASTNode **nodes, int count)
{
    for (int i = 0; i < count; ++i)
    {
        ASTNode *n = nodes[i];

        if (n->type == NODE_SET)
        {
            Value result;
            if (n->is_copy)
            {
                if (n->copy_from_attr)
                {
                    result = resolve_attribute_chain(n->copy_from_attr);
                }
                else
                {
                    result = get_variable(n->copy_from_var);
                }
            }
            else
            {
                result = clone_value(&n->literal_value);
            }

            if (n->set_attr)
            {
                assign_attribute_chain(n->set_attr, result);
            }
            else
            {
                set_variable(n->set_name, result);
            }
        }

        else if (n->type == NODE_FUNC_CALL)
        {
            if (strcmp(n->func_name, "pr") == 0)
            {
                for (int j = 0; j < n->child_count; ++j)
                {
                    ASTNode *arg = n->children[j];
                    Value val;

                    if (arg->type == NODE_ATTR_ACCESS)
                    {
                        val = resolve_attribute_chain(arg);
                    }
                    else if (arg->type == NODE_VAR)
                    {
                        val = get_variable(arg->set_name);
                    }
                    else if (arg->type == NODE_LITERAL)
                    {
                        val = arg->literal_value;
                    }
                    else
                    {
                        log_error("pr() received unsupported argument type");
                        goto pr_end;
                    }

                    print_value(val, 0);

                    if (j < n->child_count - 1)
                        printf(" ");
                }

            pr_end:
                printf("\n");
                continue;
            }

            else
            {
                log_error("Unknown function: %s", n->func_name);
            }
        }
    }
}
