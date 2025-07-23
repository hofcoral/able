#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/variable.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"

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
                if (n->child_count != 1)
                {
                    fprintf(stderr, "pr() takes 1 argument\n");
                    continue;
                }

                ASTNode *arg = n->children[0];

                if (arg->type == NODE_ATTR_ACCESS)
                {
                    Value obj_val = get_variable(arg->object_name);

                    if (obj_val.type != VAL_OBJECT)
                    {
                        fprintf(stderr, "Error: %s is not an object\n", arg->object_name);
                        continue;
                    }

                    Object *obj = obj_val.obj;
                    Value attr_val = object_get(obj, arg->attr_name);

                    print_value(attr_val, 0);
                    printf("\n");
                    continue;
                }

                else if (arg->type == NODE_VAR)
                {
                    Value val = get_variable(arg->set_name);
                    print_value(val, 0);
                    printf("\n");
                    continue;
                }

                else
                {
                    fprintf(stderr, "pr() only supports variables or attributes\n");
                    continue;
                }
            }

            else
            {
                fprintf(stderr, "Unknown function: %s\n", n->func_name);
            }
        }
    }
}
