#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/env.h"
#include "types/function.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"
#include "utils/utils.h"

static Value exec_func_call(ASTNode *n);

static double to_number(Value v)
{
    switch (v.type)
    {
    case VAL_NUMBER:
        return v.num;
    case VAL_BOOL:
        return v.boolean ? 1 : 0;
    case VAL_STRING:
        return atof(v.str);
    default:
        return NAN;
    }
}

static bool to_boolean(Value v)
{
    switch (v.type)
    {
    case VAL_BOOL:
        return v.boolean;
    case VAL_NUMBER:
        return v.num != 0;
    case VAL_STRING:
        return v.str && v.str[0] != '\0';
    case VAL_NULL:
    case VAL_UNDEFINED:
        return false;
    default:
        return true;
    }
}

static bool strict_equal(Value a, Value b)
{
    if (a.type != b.type)
        return false;

    switch (a.type)
    {
    case VAL_NUMBER:
        return a.num == b.num;
    case VAL_STRING:
        return strcmp(a.str, b.str) == 0;
    case VAL_BOOL:
        return a.boolean == b.boolean;
    case VAL_OBJECT:
        return a.obj == b.obj;
    case VAL_FUNCTION:
        return a.func == b.func;
    case VAL_NULL:
    case VAL_UNDEFINED:
        return true;
    default:
        return false;
    }
}

static bool loose_equal(Value a, Value b)
{
    if (a.type == b.type)
        return strict_equal(a, b);

    if ((a.type == VAL_NUMBER || a.type == VAL_STRING || a.type == VAL_BOOL) &&
        (b.type == VAL_NUMBER || b.type == VAL_STRING || b.type == VAL_BOOL))
    {
        double na = to_number(a);
        double nb = to_number(b);
        return na == nb;
    }

    return false;
}

void interpreter_set_env(Env *env)
{
    current_env = env;
}

static Value eval_node(ASTNode *n)
{
    switch (n->type)
    {
    case NODE_VAR:
        return get_variable(current_env, n->set_name, n->line);
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
        if (n->binary_op == OP_EQ || n->binary_op == OP_STRICT_EQ)
        {
            bool eq = n->binary_op == OP_EQ ? loose_equal(left, right)
                                            : strict_equal(left, right);
            Value res = {.type = VAL_BOOL, .boolean = eq};
            return res;
        }

        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
        {
            Value res = {.type = VAL_NUMBER};
            switch (n->binary_op)
            {
            case OP_ADD:
                res.num = left.num + right.num;
                break;
            case OP_SUB:
                res.num = left.num - right.num;
                break;
            case OP_MUL:
                res.num = left.num * right.num;
                break;
            case OP_DIV:
                res.num = right.num != 0 ? left.num / right.num : 0;
                break;
            case OP_MOD:
                res.num = fmod(left.num, right.num);
                break;
            default:
                log_script_error(n->line, "Unknown operator");
                exit(1);
            }
            return res;
        }
        if (n->binary_op == OP_ADD && left.type == VAL_STRING && right.type == VAL_STRING)
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

        log_script_error(n->line, "Type error in binary expression");
        exit(1);
    }
    case NODE_IF:
    {
        Value cond = eval_node(n->children[0]);
        if (to_boolean(cond))
        {
            return run_ast(n->children[1]->children, n->children[1]->child_count);
        }
        if (n->child_count >= 3)
        {
            ASTNode *else_node = n->children[2];
            if (else_node->type == NODE_IF)
                return eval_node(else_node);
            return run_ast(else_node->children, else_node->child_count);
        }
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }
    case NODE_BLOCK:
        return run_ast(n->children, n->child_count);
    default:
        log_script_error(n->line, "Unsupported eval node type");
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

    if (n->func_callee->type == NODE_VAR && strcmp(n->func_callee->set_name, "type") == 0)
    {
        if (n->child_count != 1)
        {
            log_script_error(n->line, "type() expects exactly one argument");
            exit(1);
        }
        Value arg = eval_node(n->children[0]);
        const char *name = value_type_name(arg.type);
        Value res = {.type = VAL_STRING, .str = strdup(name)};
        return res;
    }

    if (n->func_callee->type == NODE_VAR && strcmp(n->func_callee->set_name, "bool") == 0)
    {
        if (n->child_count != 1)
        {
            log_script_error(n->line, "bool() expects exactly one argument");
            exit(1);
        }
        Value arg = eval_node(n->children[0]);
        Value res = {.type = VAL_BOOL, .boolean = to_boolean(arg)};
        return res;
    }

    Value callee_val = eval_node(n->func_callee);
    if (callee_val.type != VAL_FUNCTION)
    {
        log_script_error(n->line, "Attempting to call non-function");
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }

    // Check correct arguments
    Function *fn = callee_val.func;

    if (callee_val.func->param_count != n->child_count)
    {
        log_script_error(n->line, "Function '%s' expects %d arguments, but got %d",
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
        else if (n->type == NODE_IF)
        {
            Value cond = eval_node(n->children[0]);
            if (to_boolean(cond))
            {
                last = run_ast(n->children[1]->children, n->children[1]->child_count);
            }
            else if (n->child_count >= 3)
            {
                ASTNode *else_node = n->children[2];
                if (else_node->type == NODE_IF)
                {
                    Value res = eval_node(else_node);
                    last = res;
                }
                else
                {
                    last = run_ast(else_node->children, else_node->child_count);
                }
            }
        }
        else if (n->type == NODE_RETURN)
        {
            last = eval_node(n->children[0]);
            return last;
        }
        else if (n->type == NODE_BLOCK)
        {
            last = run_ast(n->children, n->child_count);
        }
    }

    return last;
}
