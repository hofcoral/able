#include <stdlib.h>

#include "types/env.h"
#include "types/function.h"
#include "types/instance.h"
#include "types/promise.h"
#include "types/type.h"
#include "utils/utils.h"
#include "interpreter/attr.h"
#include "interpreter/stack.h"
#include "interpreter/interpreter.h"

extern CallStack call_stack;

static Value run_async_task(AsyncTask *task)
{
    Function *fn = task->fn;
    Env *env = env_create(fn->env);
    int offset = 0;
    if (task->has_self)
    {
        set_variable(env, fn->params[0], task->self);
        offset = 1;
    }
    for (int i = 0; i < task->arg_count; ++i)
        set_variable(env, fn->params[i + offset], task->args[i]);

    CallFrame frame = {.env = env, .return_ptr = NULL, .returning = false};
    push_frame(&call_stack, frame);
    Value result = run_ast(fn->body, fn->body_count);
    pop_frame(&call_stack);
    env_release(env);
    return result;
}

Value interpreter_create_async_promise(Function *fn, Value *args, int arg_count, bool has_self, Value self, int line, int column)
{
    AsyncTask *task = async_task_create(fn, args, arg_count, has_self, self, line, column);
    Promise *promise = promise_create_with_task(task);
    Value promise_val = {.type = VAL_PROMISE, .promise = promise};
    return promise_val;
}

Value interpreter_await(Value awaited, int line, int column)
{
    Value current = clone_value(&awaited);
    while (current.type == VAL_PROMISE)
    {
        Promise *promise = current.promise;
        PromiseState state = promise_state(promise);

        if (state == PROMISE_PENDING)
        {
            AsyncTask *task = promise_take_task(promise);
            if (task)
            {
                Value result = run_async_task(task);
                promise_resolve(promise, result);
                free_value(result);
                async_task_free(task);
                state = promise_state(promise);
            }
            else
            {
                free_value(current);
                log_script_error(line, column, "Promise is still pending");
                exit(1);
            }
        }

        if (state == PROMISE_FULFILLED)
        {
            Value next = promise_clone_result(promise);
            if (next.type == VAL_PROMISE && next.promise == promise)
            {
                free_value(next);
                free_value(current);
                log_script_error(line, column, "Promise resolved with itself");
                exit(1);
            }
            free_value(current);
            current = next;
            continue;
        }

        if (state == PROMISE_REJECTED)
        {
            Value reason = promise_clone_reason(promise);
            free_value(current);
            if (reason.type == VAL_STRING)
                log_script_error(line, column, "Promise rejected: %s", reason.str);
            else
                log_script_error(line, column, "Promise rejected");
            free_value(reason);
            exit(1);
        }

        break;
    }

    return current;
}

Value interpreter_call_value(Value callee, Value *args, int arg_count, int line, int column)
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
        Value self_val = {.type = VAL_INSTANCE, .instance = callee.bound->self};
        if (fn->is_async)
            return interpreter_create_async_promise(fn, args, arg_count, true, self_val, line, column);
        Env *env = env_create(fn->env);
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
    if (callee.type == VAL_TYPE && promise_type_is_namespace(callee.cls))
    {
        log_script_error(line, column, "Promise cannot be instantiated directly");
        exit(1);
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
    if (fn->is_async)
    {
        Value undef = {.type = VAL_UNDEFINED};
        return interpreter_create_async_promise(fn, args, arg_count, false, undef, line, column);
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

Value interpreter_call_and_await(Value callee, Value *args, int arg_count, int line, int column)
{
    Value result = interpreter_call_value(callee, args, arg_count, line, column);
    if (result.type != VAL_PROMISE)
        return result;

    Value awaited = interpreter_await(result, line, column);
    free_value(result);
    return awaited;
}
