#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "ast/ast.h"
#include "types/env.h"
#include "types/function.h"
#include "types/list.h"
#include "types/type.h"
#include "types/instance.h"
#include "interpreter/attr.h"
#include "interpreter/resolve.h"
#include "interpreter/interpreter.h"
#include "interpreter/stack.h"
#include "utils/utils.h"
#include "types/type_registry.h"

static CallStack call_stack;
static bool break_flag = false;
static bool continue_flag = false;

static Value call_value(Value callee, Value *args, int arg_count, int line, int column)
{
    if (callee.type == VAL_BOUND_METHOD)
    {
        Function *fn = callee.bound->func;
        if (fn->param_count - 1 != arg_count)
        {
            log_script_error(line, column,
                             "Function expects %d arguments, but got %d",
                             fn->param_count - 1, arg_count);
            exit(1);
        }
        Env *env = env_create(fn->env);
        Value self_val = {.type = VAL_INSTANCE, .instance = callee.bound->self};
        set_variable(env, fn->params[0], self_val);
        for (int p = 0; p < arg_count; ++p)
            set_variable(env, fn->params[p + 1], args[p]);
        CallFrame frame = {.env = env, .return_ptr = NULL, .returning = false};
        push_frame(&call_stack, frame);
        Value result = run_ast(fn->body, fn->body_count);
        pop_frame(&call_stack);
        Value ret_val = clone_value(&result);
        env_release(env);
        return ret_val;
    }
    if (callee.type == VAL_TYPE)
    {
        Instance *inst = instance_create(callee.cls);
        Value inst_val = {.type = VAL_INSTANCE, .instance = inst};
        Value init = value_get_attr(inst_val, "init");
        if (init.type == VAL_BOUND_METHOD)
        {
            Function *fn = init.bound->func;
            if (fn->param_count - 1 != arg_count)
            {
                log_script_error(line, column, "init expects %d arguments, got %d",
                                 fn->param_count - 1, arg_count);
                exit(1);
            }
            Env *env = env_create(fn->env);
            Value selfv = {.type = VAL_INSTANCE, .instance = init.bound->self};
            set_variable(env, fn->params[0], selfv);
            for (int p = 0; p < arg_count; ++p)
                set_variable(env, fn->params[p + 1], args[p]);
            CallFrame frame = {.env = env, .return_ptr = NULL, .returning = false};
            push_frame(&call_stack, frame);
            run_ast(fn->body, fn->body_count);
            pop_frame(&call_stack);
            env_release(env);
        }
        return inst_val;
    }
    if (callee.type != VAL_FUNCTION)
    {
        log_script_error(line, column, "Attempting to call non-function");
        exit(1);
    }
    Function *fn = callee.func;
    if (fn->param_count != arg_count)
    {
        log_script_error(line, column, "Function expects %d arguments, but got %d",
                         fn->param_count, arg_count);
        exit(1);
    }
    Env *env = env_create(fn->env);
    for (int p = 0; p < arg_count; ++p)
        set_variable(env, fn->params[p], args[p]);
    CallFrame frame = {.env = env, .return_ptr = NULL, .returning = false};
    push_frame(&call_stack, frame);
    Value result = run_ast(fn->body, fn->body_count);
    pop_frame(&call_stack);
    Value ret_val = clone_value(&result);
    env_release(env);
    return ret_val;
}

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
        return get_variable(interpreter_current_env(), n->data.set.set_name, n->line, n->column);
    case NODE_ATTR_ACCESS:
        return resolve_attribute_chain(n);
    case NODE_LITERAL:
        return clone_value(&n->data.lit.literal_value);
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
    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.set.set_name, "pr") == 0)
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

    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.set.set_name, "type") == 0)
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

    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.set.set_name, "bool") == 0)
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

    if (n->data.call.func_callee->type == NODE_VAR && strcmp(n->data.call.func_callee->data.set.set_name, "list") == 0)
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
    if (callee_val.type == VAL_BOUND_METHOD)
    {
        Function *fn = callee_val.bound->func;
        if (fn->param_count - 1 != n->child_count)
        {
            log_script_error(n->line, n->column,
                             "Function '%s' expects %d arguments, but got %d",
                             n->data.call.func_name,
                             fn->param_count - 1, n->child_count);
            exit(1);
        }

        Env *call_env = env_create(fn->env);
        Value self_val = {.type = VAL_INSTANCE, .instance = callee_val.bound->self};
        set_variable(call_env, fn->params[0], self_val);
        for (int p = 0; p < n->child_count; ++p)
        {
            Value arg_val = eval_node(n->children[p]);
            set_variable(call_env, fn->params[p + 1], arg_val);
        }
        CallFrame frame = {.env = call_env, .return_ptr = NULL, .returning = false};
        push_frame(&call_stack, frame);
        Value result = run_ast(fn->body, fn->body_count);
        pop_frame(&call_stack);
        Value ret_val = clone_value(&result);
        env_release(call_env);
        return ret_val;
    }
    if (callee_val.type == VAL_TYPE)
    {
        Instance *inst = instance_create(callee_val.cls);
        Value inst_val = {.type = VAL_INSTANCE, .instance = inst};
        Value init = value_get_attr(inst_val, "init");
        if (init.type == VAL_BOUND_METHOD)
        {
            Function *fn = init.bound->func;
            if (fn->param_count - 1 != n->child_count)
            {
                log_script_error(n->line, n->column,
                                 "init expects %d arguments, got %d",
                                 fn->param_count - 1, n->child_count);
                exit(1);
            }
            Env *env = env_create(fn->env);
            Value selfv = {.type = VAL_INSTANCE, .instance = init.bound->self};
            set_variable(env, fn->params[0], selfv);
            for (int p = 0; p < n->child_count; ++p)
            {
                Value arg_val = eval_node(n->children[p]);
                set_variable(env, fn->params[p + 1], arg_val);
            }
            CallFrame frame = {.env = env, .return_ptr = NULL, .returning = false};
            push_frame(&call_stack, frame);
            run_ast(fn->body, fn->body_count);
            pop_frame(&call_stack);
            env_release(env);
        }
        return inst_val;
    }
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
        switch (n->type)
        {
        case NODE_SET:
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
            break;
        }
        case NODE_CLASS_DEF:
        {
            int base_count = n->data.cls.base_count;
            Type **bases = NULL;
            if (base_count > 0)
            {
                bases = malloc(sizeof(Type *) * base_count);
                for (int i = 0; i < base_count; ++i)
                {
                    Value bv = get_variable(interpreter_current_env(), n->data.cls.base_names[i], n->line, n->column);
                    if (bv.type != VAL_TYPE)
                    {
                        log_script_error(n->line, n->column, "Unknown base type '%s'", n->data.cls.base_names[i]);
                        exit(1);
                    }
                    bases[i] = bv.cls;
                }
            }

            Type *t = type_create(n->data.cls.class_name);
            type_set_bases(t, bases, base_count);

            for (int m = 0; m < n->child_count; ++m)
            {
                ASTNode *method = n->children[m];
                Function *fn = malloc(sizeof(Function));
                fn->name = strdup(method->data.method.method_name);
                fn->param_count = method->data.method.param_count;
                fn->params = malloc(sizeof(char *) * fn->param_count);
                for (int p = 0; p < fn->param_count; ++p)
                    fn->params[p] = strdup(method->data.method.params[p]);
                fn->body = method->children;
                fn->body_count = method->child_count;
                fn->env = interpreter_current_env();
                env_retain(interpreter_current_env());
                fn->bind_on_access = !method->is_static;
                Value fv = {.type = VAL_FUNCTION, .func = fn};
                object_set(t->attributes, method->data.method.method_name, fv);
            }

            Value tv = {.type = VAL_TYPE, .cls = t};
            set_variable(interpreter_current_env(), n->data.cls.class_name, tv);
            break;
        }
        case NODE_FUNC_CALL:
            exec_func_call(n);
            break;
        case NODE_IF:
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
            break;
        }
        case NODE_RETURN:
        {
            last = eval_node(n->children[0]);
            CallFrame *f = current_frame(&call_stack);
            if (f)
                f->returning = true;
            return last;
        }
        case NODE_BLOCK:
            last = run_ast(n->children, n->child_count);
            break;
        case NODE_FOR:
        {
            Value iterable = eval_node(n->children[0]);
            ASTNode *body = n->children[1];
            if (iterable.type == VAL_LIST)
            {
                for (int i = 0; i < iterable.list->count; ++i)
                {
                    set_variable(interpreter_current_env(), n->data.loop.loop_var,
                                 iterable.list->items[i]);
                    last = run_ast(body->children, body->child_count);
                    CallFrame *cf = current_frame(&call_stack);
                    if (cf && cf->returning)
                        break;
                    if (break_flag)
                    {
                        break_flag = false;
                        break;
                    }
                    if (continue_flag)
                    {
                        continue_flag = false;
                        continue;
                    }
                }
            }
            else
            {
                Value iter_func = value_get_attr(iterable, "__iter__");
                if (iter_func.type == VAL_UNDEFINED || iter_func.type == VAL_NULL)
                {
                    log_script_error(n->line, n->column, "Object is not iterable");
                    exit(1);
                }
                Value iterator = call_value(iter_func, NULL, 0, n->line, n->column);
                while (1)
                {
                    Value next_f = value_get_attr(iterator, "__next__");
                    if (next_f.type == VAL_UNDEFINED || next_f.type == VAL_NULL)
                    {
                        log_script_error(n->line, n->column,
                                         "Iterator missing __next__ method");
                        exit(1);
                    }
                    Value item = call_value(next_f, NULL, 0, n->line, n->column);
                    if (item.type == VAL_UNDEFINED)
                        break;
                    set_variable(interpreter_current_env(), n->data.loop.loop_var,
                                 item);
                    last = run_ast(body->children, body->child_count);
                    CallFrame *cf = current_frame(&call_stack);
                    if (cf && cf->returning)
                        break;
                    if (break_flag)
                    {
                        break_flag = false;
                        break;
                    }
                    if (continue_flag)
                    {
                        continue_flag = false;
                        continue;
                    }
                }
            }
            break;
        }
        case NODE_WHILE:
        {
            while (to_boolean(eval_node(n->children[0])))
            {
                last = run_ast(n->children[1]->children, n->children[1]->child_count);
                CallFrame *cf = current_frame(&call_stack);
                if (cf && cf->returning)
                    break;
                if (break_flag)
                {
                    break_flag = false;
                    break;
                }
                if (continue_flag)
                {
                    continue_flag = false;
                    continue;
                }
            }
            break;
        }
        case NODE_BREAK:
            break_flag = true;
            return last;
        case NODE_CONTINUE:
            continue_flag = true;
            return last;
        default:
            break;
        }

        CallFrame *cf = current_frame(&call_stack);
        if (cf && cf->returning)
            return last;
        if (break_flag || continue_flag)
            return last;
    }

    return last;
}
