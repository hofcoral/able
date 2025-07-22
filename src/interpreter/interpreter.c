#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/variable.h"
#include "ast/ast.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"

void run_ast(ASTNode **nodes, int count)
{
    for (int i = 0; i < count; ++i)
    {
        ASTNode *n = nodes[i];
        // Set stmt
        if (n->type == NODE_SET)
        {
            Value result;
            if (n->is_copy)
            {
                result = get_variable(n->copy_from_var); // Clone this if needed
            }
            else
            {
                result = clone_value(&n->literal_value);
            }

            set_variable(n->set_name, result);
        }

        // Function call
        else if (n->type == NODE_FUNC_CALL)
        {
            if (strcmp(n->func_name, "pr") == 0)
            {
                if (n->arg_count != 1)
                {
                    fprintf(stderr, "pr() takes 1 argument\n");
                    continue;
                }
                ASTNode *arg = n->args[0];
                if (arg->type != NODE_VAR)
                {
                    fprintf(stderr, "pr() only supports variables for now\n");
                    continue;
                }
                Value val = get_variable(arg->set_name);
                if (val.type == VAL_STRING)
                {
                    printf("%s\n", val.str);
                }
                else if (val.type == VAL_NUMBER)
                {
                    printf("%f\n", val.num);
                }
                else
                {
                    printf("(undefined: %s)\n", arg->set_name);
                }
            }
            else
            {
                fprintf(stderr, "Unknown function: %s\n", n->func_name);
            }
        }
    }
}
