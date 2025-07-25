#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/env.h"
#include "types/function.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"
#include "utils/utils.h"

static Value exec_func_call(ASTNode *n);

void interpreter_set_env(Env *env)
{
    current_env = env;
}

static Value eval_node(ASTNode *n)
{
    switch (n->type)
    {
    case NODE_VAR:
        return get_variable(current_env, n->set_name);
    case NODE_ATTR_ACCESS:
        return resolve_attribute_chain(n);
    case NODE_LITERAL:
        return clone_value(&n->literal_value);
    case NODE_FUNC_CALL:
        return exec_func_call(n);
    case NODE_BINARY:
    {
        Value left = eval_node(n->children[0]);
        Value right = eval_node(n->children[1]);
        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
        {
            Value res = {.type = VAL_NUMBER};
            switch (n->binary_op)
            {
            case '+':
                res.num = left.num + right.num;
                break;
            case '-':
                res.num = left.num - right.num;
                break;
            case '*':
                res.num = left.num * right.num;
                break;
            case '/':
                res.num = right.num != 0 ? left.num / right.num : 0;
                break;
            case '%':
                res.num = fmod(left.num, right.num);
                break;
            default:
                log_error("Unknown operator");
                exit(1);
            }
            return res;
        }
        if (n->binary_op == '+' && left.type == VAL_STRING && right.type == VAL_STRING)
        {
            size_t len1 = strlen(left.str);
            size_t len2 = strlen(right.str);
            char *buf = malloc(len1 + len2 + 1);
            memcpy(buf, left.str, len1);
            memcpy(buf + len1, right.str, len2);
            buf[len1 + len2] = '\0';
            Value res = {.type = VAL_STRING, .str = buf};
            return res;
        }

        log_error("Type error in binary expression");
        exit(1);
    }
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

    Env *call_env = env_create(fn->env);
    for (int p = 0; p < fn->param_count && p < n->child_count; ++p)
    {
        Value arg_val = eval_node(n->children[p]);
        set_variable(call_env, fn->params[p], arg_val);
    }
    Env *prev_env = current_env;
    current_env = call_env;
    Value result = run_ast(fn->body, fn->body_count);
    current_env = prev_env;
    Value ret_val = clone_value(&result);
    env_release(call_env);
    return ret_val;
}

Value run_ast(ASTNode **nodes, int count)
{
    Value last = {.type = VAL_UNDEFINED};
    for (int i = 0; i < count; ++i)
    {
        ASTNode *n = nodes[i];

        if (n->type == NODE_SET)
        {
            Value result = eval_node(n->children[0]);

            if (n->set_attr)
            {
                assign_attribute_chain(n->set_attr, result);
            }
            else
            {
                if (result.type == VAL_FUNCTION && result.func->env == NULL)
                {
                    result.func->env = current_env;
                    env_retain(current_env);
                }
                set_variable(current_env, n->set_name, result);
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
