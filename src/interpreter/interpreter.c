#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/env.h"
#include "types/function.h"
#include "types/list.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"
#include "interpreter/stack.h"
#include "utils/utils.h"
#include "types/type_registry.h"

static CallStack call_stack;

static Value exec_func_call(ASTNode *n);
static Value resolve_attr_prefix(ASTNode *attr_node, int count);

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
    case VAL_LIST:
        return a.list == b.list;
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

static Value resolve_attr_prefix(ASTNode *attr_node, int count)
{
    Value base = get_variable(interpreter_current_env(), attr_node->data.attr.object_name,
                              attr_node->line, attr_node->column);
    for (int i = 0; i < count; ++i)
    {
        if (base.type != VAL_OBJECT)
        {
            log_script_error(attr_node->children[i]->line,
                              attr_node->children[i]->column,
                              "Error: intermediate '%s' is not an object",
                              attr_node->children[i]->data.attr.attr_name);
            exit(1);
        }
        base = object_get(base.obj, attr_node->children[i]->data.attr.attr_name);
    }
    return base;
}

void interpreter_init()
{
    stack_init(&call_stack);
    type_registry_init();
}

void interpreter_cleanup()
{
    type_registry_cleanup();
    stack_free(&call_stack);
}

void interpreter_set_env(Env *env)
{
    CallFrame frame = {.env = env, .return_ptr = NULL, .returning = false};
    push_frame(&call_stack, frame);
}

Env *interpreter_current_env()
{
    CallFrame *frame = current_frame(&call_stack);
    if (!frame)
        return NULL;
    return frame->env;
}

static Value eval_node(ASTNode *n)
{
    switch (n->type)
    {
    case NODE_VAR:
        return get_variable(interpreter_current_env(), n->data.var.var_name, n->line, n->column);
    case NODE_ATTR_ACCESS:
        return resolve_attribute_chain(n);
    case NODE_LITERAL:
        return clone_value(&n->data.literal);
    case NODE_FUNC_CALL:
        return exec_func_call(n);
    case NODE_BINARY:
    {
        Value left = eval_node(n->children[0]);
        Value right = eval_node(n->children[1]);
        if (n->data.binary.op == OP_EQ || n->data.binary.op == OP_STRICT_EQ)
        {
            bool eq = n->data.binary.op == OP_EQ ? loose_equal(left, right)
                                                 : strict_equal(left, right);
            Value res = {.type = VAL_BOOL, .boolean = eq};
            return res;
        }

        if (n->data.binary.op == OP_LT || n->data.binary.op == OP_GT ||
            n->data.binary.op == OP_LTE || n->data.binary.op == OP_GTE)
        {
            bool cmp;
            if ((left.type == VAL_NUMBER || left.type == VAL_BOOL) &&
                (right.type == VAL_NUMBER || right.type == VAL_BOOL))
            {
                double ln = to_number(left);
                double rn = to_number(right);
                switch (n->data.binary.op)
                {
                case OP_LT:
                    cmp = ln < rn;
                    break;
                case OP_GT:
                    cmp = ln > rn;
                    break;
                case OP_LTE:
                    cmp = ln <= rn;
                    break;
                default:
                    cmp = ln >= rn;
                }
            }
            else if (left.type == VAL_STRING && right.type == VAL_STRING)
            {
                int c = strcmp(left.str, right.str);
                switch (n->data.binary.op)
                {
                case OP_LT:
                    cmp = c < 0;
                    break;
                case OP_GT:
                    cmp = c > 0;
                    break;
                case OP_LTE:
                    cmp = c <= 0;
                    break;
                default:
                    cmp = c >= 0;
                }
            }
            else
            {
                log_script_error(n->line, n->column, "Type error in binary expression");
                exit(1);
            }
            Value res = {.type = VAL_BOOL, .boolean = cmp};
            return res;
        }

        if (left.type == VAL_NUMBER && right.type == VAL_NUMBER)
        {
            Value res = {.type = VAL_NUMBER};
            switch (n->data.binary.op)
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
                log_script_error(n->line, n->column, "Unknown operator");
                exit(1);
            }
            return res;
        }
        if (n->data.binary.op == OP_ADD && left.type == VAL_STRING && right.type == VAL_STRING)
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
        if (n->data.binary.op == OP_ADD && left.type == VAL_LIST && right.type == VAL_LIST)
        {
            List *list = malloc(sizeof(List));
            list->count = 0;
            list->capacity = left.list->count + right.list->count;
            list->items = malloc(sizeof(Value) * list->capacity);
            for (int i = 0; i < left.list->count; ++i)
                list->items[list->count++] = clone_value(&left.list->items[i]);
            for (int j = 0; j < right.list->count; ++j)
                list->items[list->count++] = clone_value(&right.list->items[j]);
            Value res = {.type = VAL_LIST, .list = list};
            return res;
        }

        log_script_error(n->line, n->column, "Type error in binary expression");
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
        log_script_error(n->line, n->column, "Unsupported eval node type");
        exit(1);
    }
}

static Value exec_func_call(ASTNode *n)
{
    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.var.var_name, "pr") == 0)
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

    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.var.var_name, "type") == 0)
    {
        if (n->child_count != 1)
        {
            log_script_error(n->line, n->column, "type() expects exactly one argument");
            exit(1);
        }
        Value arg = eval_node(n->children[0]);
        const char *name = value_type_name(arg.type);
        Value res = {.type = VAL_STRING, .str = strdup(name)};
        return res;
    }

    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.var.var_name, "bool") == 0)
    {
        if (n->child_count != 1)
        {
            log_script_error(n->line, n->column, "bool() expects exactly one argument");
            exit(1);
        }
        Value arg = eval_node(n->children[0]);
        Value res = {.type = VAL_BOOL, .boolean = to_boolean(arg)};
        return res;
    }

    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.var.var_name, "list") == 0)
    {
        if (n->child_count > 1)
        {
            log_script_error(n->line, n->column, "list() expects at most one argument");
            exit(1);
        }
        List *list = malloc(sizeof(List));
        list->count = 0;
        list->capacity = 0;
        list->items = NULL;
        if (n->child_count == 1)
        {
            Value arg = eval_node(n->children[0]);
            if (arg.type == VAL_LIST)
            {
                free(list);
                list = clone_list(arg.list);
            }
            else
            {
                list_append(list, arg);
            }
        }
        Value res = {.type = VAL_LIST, .list = list};
        return res;
    }

    if (n->data.call.func_callee->type == NODE_ATTR_ACCESS)
    {
        ASTNode *attr = n->data.call.func_callee;
        if (attr->child_count > 0)
        {
            ASTNode *last = attr->children[attr->child_count - 1];
            const char *name = last->data.attr.attr_name;
            Value target = resolve_attr_prefix(attr, attr->child_count - 1);
            if (target.type == VAL_LIST)
            {
                if (strcmp(name, "append") == 0)
                {
                    if (n->child_count != 1)
                    {
                        log_script_error(n->line, n->column, "append() expects one argument");
                        exit(1);
                    }
                    Value arg = eval_node(n->children[0]);
                    list_append(target.list, arg);
                    Value undef = {.type = VAL_UNDEFINED};
                    return undef;
                }
                if (strcmp(name, "remove") == 0)
                {
                    if (n->child_count != 1)
                    {
                        log_script_error(n->line, n->column, "remove() expects one argument");
                        exit(1);
                    }
                    Value idxv = eval_node(n->children[0]);
                    if (idxv.type != VAL_NUMBER)
                    {
                        log_script_error(n->line, n->column, "remove() index must be number");
                        exit(1);
                    }
                    return list_remove(target.list, (int)idxv.num);
                }
                if (strcmp(name, "get") == 0)
                {
                    if (n->child_count != 1)
                    {
                        log_script_error(n->line, n->column, "get() expects one argument");
                        exit(1);
                    }
                    Value idxv = eval_node(n->children[0]);
                    if (idxv.type != VAL_NUMBER)
                    {
                        log_script_error(n->line, n->column, "get() index must be number");
                        exit(1);
                    }
                    Value item = list_get(target.list, (int)idxv.num);
                    return clone_value(&item);
                }
                if (strcmp(name, "extend") == 0)
                {
                    if (n->child_count != 1)
                    {
                        log_script_error(n->line, n->column, "extend() expects one argument");
                        exit(1);
                    }
                    Value lst = eval_node(n->children[0]);
                    if (lst.type != VAL_LIST)
                    {
                        log_script_error(n->line, n->column, "extend() expects a list");
                        exit(1);
                    }
                    list_extend(target.list, lst.list);
                    Value undef = {.type = VAL_UNDEFINED};
                    return undef;
                }
            }
        }
    }

    Value callee_val = eval_node(n->data.call.func_callee);
    if (callee_val.type != VAL_FUNCTION)
    {
        log_script_error(n->line, n->column, "Attempting to call non-function");
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }

    // Check correct arguments
    Function *fn = callee_val.func;

    if (callee_val.func->param_count != n->child_count)
    {
        log_script_error(n->line, n->column, "Function '%s' expects %d arguments, but got %d",
                  n->data.call.func_name,
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
    CallFrame frame = {.env = call_env, .return_ptr = NULL, .returning = false};
    push_frame(&call_stack, frame);
    Value result = run_ast(fn->body, fn->body_count);
    pop_frame(&call_stack);
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

            if (n->data.set.set_attr)
            {
                assign_attribute_chain(n->data.set.set_attr, result);
            }
            else
            {
                if (result.type == VAL_FUNCTION && result.func->env == NULL)
                {
                    result.func->env = interpreter_current_env();
                    env_retain(interpreter_current_env());
                }
                set_variable(interpreter_current_env(), n->data.set.set_name, result);
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
            CallFrame *f = current_frame(&call_stack);
            if (f)
                f->returning = true;
            return last;
        }
        else if (n->type == NODE_BLOCK)
        {
            last = run_ast(n->children, n->child_count);
        }

        CallFrame *cf = current_frame(&call_stack);
        if (cf && cf->returning)
            return last;
    }

    return last;
}
