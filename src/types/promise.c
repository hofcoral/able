#include <stdlib.h>

#include "types/promise.h"
#include "types/object.h"

static Type *PROMISE_NAMESPACE = NULL;

static void ensure_promise_namespace(void)
{
    if (!PROMISE_NAMESPACE)
        PROMISE_NAMESPACE = type_create("Promise");
}

Type *promise_namespace_type(void)
{
    ensure_promise_namespace();
    return PROMISE_NAMESPACE;
}

bool promise_type_is_namespace(Type *type)
{
    return type != NULL && type == promise_namespace_type();
}

Value promise_namespace_value(void)
{
    ensure_promise_namespace();
    Value v = {.type = VAL_TYPE, .cls = PROMISE_NAMESPACE};
    return v;
}

bool promise_value_is_namespace(Value value)
{
    return value.type == VAL_TYPE && promise_type_is_namespace(value.cls);
}

Promise *promise_create(void)
{
    Promise *promise = malloc(sizeof(Promise));
    promise->ref_count = 1;
    promise->state = PROMISE_PENDING;
    promise->result.type = VAL_UNDEFINED;
    promise->reason.type = VAL_UNDEFINED;
    promise->task = NULL;
    return promise;
}

Promise *promise_create_with_task(AsyncTask *task)
{
    Promise *promise = promise_create();
    promise->task = task;
    return promise;
}

void promise_attach_task(Promise *promise, AsyncTask *task)
{
    if (!promise)
        return;
    if (promise->task)
        async_task_free(promise->task);
    promise->task = task;
}

AsyncTask *promise_take_task(Promise *promise)
{
    if (!promise)
        return NULL;
    AsyncTask *task = promise->task;
    promise->task = NULL;
    return task;
}

PromiseState promise_state(const Promise *promise)
{
    if (!promise)
        return PROMISE_PENDING;
    return promise->state;
}

void promise_resolve(Promise *promise, Value value)
{
    if (!promise)
        return;
    if (promise->result.type != VAL_UNDEFINED)
        free_value(promise->result);
    if (promise->reason.type != VAL_UNDEFINED)
    {
        free_value(promise->reason);
        promise->reason.type = VAL_UNDEFINED;
    }
    promise->result = clone_value(&value);
    promise->state = PROMISE_FULFILLED;
}

void promise_reject(Promise *promise, Value reason)
{
    if (!promise)
        return;
    if (promise->reason.type != VAL_UNDEFINED)
        free_value(promise->reason);
    if (promise->result.type != VAL_UNDEFINED)
    {
        free_value(promise->result);
        promise->result.type = VAL_UNDEFINED;
    }
    promise->reason = clone_value(&reason);
    promise->state = PROMISE_REJECTED;
}

Value promise_clone_result(const Promise *promise)
{
    if (!promise || promise->state != PROMISE_FULFILLED)
    {
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }
    return clone_value(&promise->result);
}

Value promise_clone_reason(const Promise *promise)
{
    if (!promise || promise->state != PROMISE_REJECTED)
    {
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }
    return clone_value(&promise->reason);
}

void promise_retain(Promise *promise)
{
    if (promise)
        promise->ref_count++;
}

void promise_release(Promise *promise)
{
    if (!promise)
        return;
    if (--promise->ref_count > 0)
        return;
    if (promise->result.type != VAL_UNDEFINED)
        free_value(promise->result);
    if (promise->reason.type != VAL_UNDEFINED)
        free_value(promise->reason);
    if (promise->task)
        async_task_free(promise->task);
    free(promise);
}

AsyncTask *async_task_create(Function *fn, Value *args, int arg_count, bool has_self, Value self, int line, int column)
{
    AsyncTask *task = malloc(sizeof(AsyncTask));
    task->fn = fn;
    task->arg_count = arg_count;
    task->args = NULL;
    if (arg_count > 0)
    {
        task->args = malloc(sizeof(Value) * arg_count);
        for (int i = 0; i < arg_count; ++i)
            task->args[i] = clone_value(&args[i]);
    }
    task->has_self = has_self;
    if (has_self)
        task->self = clone_value(&self);
    else
        task->self.type = VAL_UNDEFINED;
    task->line = line;
    task->column = column;
    return task;
}

void async_task_free(AsyncTask *task)
{
    if (!task)
        return;
    if (task->args)
    {
        for (int i = 0; i < task->arg_count; ++i)
            free_value(task->args[i]);
        free(task->args);
    }
    if (task->has_self && task->self.type != VAL_UNDEFINED)
        free_value(task->self);
    free(task);
}
