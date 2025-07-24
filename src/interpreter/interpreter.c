#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/variable.h"
#include "types/function.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"
#include "utils/utils.h"

static Value exec_func_call(ASTNode *n);

static Value eval_node(ASTNode *n)
{
    switch (n->type)
    {
    case NODE_VAR:
        return get_variable(n->set_name);
    case NODE_ATTR_ACCESS:
        return resolve_attribute_chain(n);
    case NODE_LITERAL:
        return clone_value(&n->literal_value);
    case NODE_FUNC_CALL:
        return exec_func_call(n);
    default:
        log_error("Unsupported eval node type");
        exit(1);
    }
}

static Value exec_func_call(ASTNode *n)
{
    if (n->func_callee->type == NODE_VAR && strcmp(n->func_callee->set_name, "pr") == 0)
    {
        for (int j = 0; j < n->child_count; ++j)
        {
            Value val = eval_node(n->children[j]);
            print_value(val, 0);
            if (j < n->child_count - 1)
                printf(" ");
        }
        printf("\n");
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }

    Value callee_val = eval_node(n->func_callee);
    if (callee_val.type != VAL_FUNCTION)
    {
        log_error("Attempting to call non-function");
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }

    // Check correct arguments
    Function *fn = callee_val.func;

    if (callee_val.func->param_count != n->child_count)
    {
        log_error("Function '%s' expects %d arguments, but got %d",
                  n->func_name,
                  fn->param_count,
                  n->child_count);
        exit(1);
    }

    for (int p = 0; p < fn->param_count && p < n->child_count; ++p)
    {
        Value arg_val = eval_node(n->children[p]);
        set_variable(fn->params[p], arg_val);
    }

    return run_ast(fn->body, fn->body_count);
}

Value run_ast(ASTNode **nodes, int count)
{
    Value last = {.type = VAL_UNDEFINED};
    for (int i = 0; i < count; ++i)
    {
        ASTNode *n = nodes[i];

        if (n->type == NODE_SET)
        {
            Value result;
            if (n->is_copy)
            {
                if (n->copy_from_attr)
                    result = eval_node(n->copy_from_attr);
                else
                {
                    ASTNode temp = {.type = NODE_VAR, .set_name = n->copy_from_var};
                    result = eval_node(&temp);
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
            exec_func_call(n);
        }
        else if (n->type == NODE_RETURN)
        {
            last = eval_node(n->children[0]);
            return last;
        }
    }

    return last;
}
