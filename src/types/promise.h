#ifndef PROMISE_H
#define PROMISE_H

#include <stdbool.h>

#include "types/value.h"
#include "types/function.h"
#include "types/type.h"

typedef enum
{
    PROMISE_PENDING,
    PROMISE_FULFILLED,
    PROMISE_REJECTED
} PromiseState;

typedef struct AsyncTask
{
    Function *fn;
    Value *args;
    int arg_count;
    bool has_self;
    Value self;
    int line;
    int column;
} AsyncTask;

typedef struct Promise
{
    int ref_count;
    PromiseState state;
    Value result;
    Value reason;
    AsyncTask *task;
} Promise;

Promise *promise_create(void);
Promise *promise_create_with_task(AsyncTask *task);
void promise_attach_task(Promise *promise, AsyncTask *task);
AsyncTask *promise_take_task(Promise *promise);
PromiseState promise_state(const Promise *promise);
void promise_resolve(Promise *promise, Value value);
void promise_reject(Promise *promise, Value reason);
Value promise_clone_result(const Promise *promise);
Value promise_clone_reason(const Promise *promise);
void promise_retain(Promise *promise);
void promise_release(Promise *promise);

AsyncTask *async_task_create(Function *fn, Value *args, int arg_count, bool has_self, Value self, int line, int column);
void async_task_free(AsyncTask *task);

Type *promise_namespace_type(void);
bool promise_type_is_namespace(Type *type);
Value promise_namespace_value(void);
bool promise_value_is_namespace(Value value);

#endif
