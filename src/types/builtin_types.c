#include <stdlib.h>

#include "types/builtin_types.h"
#include "types/env.h"
#include "types/instance.h"
#include "types/promise.h"

static Type *NUMBER_TYPE = NULL;
static Type *STRING_TYPE = NULL;
static Type *BOOLEAN_TYPE = NULL;
static Type *LIST_TYPE = NULL;
static Type *OBJECT_TYPE = NULL;
static Type *FUNCTION_TYPE = NULL;
static Type *BOUND_METHOD_TYPE = NULL;
static Type *TYPE_TYPE = NULL;
static Type *INSTANCE_TYPE = NULL;
static Type *NULL_TYPE = NULL;
static Type *UNDEFINED_TYPE = NULL;

static Type *ensure_type(Type **slot, const char *name)
{
    if (!*slot)
        *slot = type_create(name);
    return *slot;
}

Type *builtin_type_number(void)
{
    return ensure_type(&NUMBER_TYPE, "Number");
}

Type *builtin_type_string(void)
{
    return ensure_type(&STRING_TYPE, "String");
}

Type *builtin_type_boolean(void)
{
    return ensure_type(&BOOLEAN_TYPE, "Boolean");
}

Type *builtin_type_list(void)
{
    return ensure_type(&LIST_TYPE, "List");
}

Type *builtin_type_object(void)
{
    return ensure_type(&OBJECT_TYPE, "Object");
}

Type *builtin_type_function(void)
{
    return ensure_type(&FUNCTION_TYPE, "Function");
}

Type *builtin_type_bound_method(void)
{
    return ensure_type(&BOUND_METHOD_TYPE, "BoundMethod");
}

Type *builtin_type_type(void)
{
    return ensure_type(&TYPE_TYPE, "Type");
}

Type *builtin_type_instance(void)
{
    return ensure_type(&INSTANCE_TYPE, "Instance");
}

Type *builtin_type_null(void)
{
    return ensure_type(&NULL_TYPE, "Null");
}

Type *builtin_type_undefined(void)
{
    return ensure_type(&UNDEFINED_TYPE, "Undefined");
}

bool type_is_subclass(Type *type, Type *base)
{
    if (!type || !base)
        return false;
    if (type == base)
        return true;
    for (int i = 0; i < type->base_count; ++i)
    {
        if (type_is_subclass(type->bases[i], base))
            return true;
    }
    return false;
}

bool value_is_type(Value value, Type *type)
{
    if (!type)
        return false;
    switch (value.type)
    {
    case VAL_NUMBER:
        return type == builtin_type_number();
    case VAL_STRING:
        return type == builtin_type_string();
    case VAL_BOOL:
        return type == builtin_type_boolean();
    case VAL_LIST:
        return type == builtin_type_list();
    case VAL_OBJECT:
        return type == builtin_type_object();
    case VAL_FUNCTION:
        return type == builtin_type_function();
    case VAL_BOUND_METHOD:
        return type == builtin_type_bound_method();
    case VAL_TYPE:
        return type == builtin_type_type();
    case VAL_NULL:
        return type == builtin_type_null();
    case VAL_UNDEFINED:
        return type == builtin_type_undefined();
    case VAL_PROMISE:
        return type == promise_namespace_type();
    case VAL_INSTANCE:
        if (type == builtin_type_instance())
            return true;
        return type_is_subclass(value.instance->cls, type);
    default:
        return false;
    }
}

Value builtin_type_value_for(Value value)
{
    switch (value.type)
    {
    case VAL_NUMBER:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_number()};
    case VAL_STRING:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_string()};
    case VAL_BOOL:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_boolean()};
    case VAL_LIST:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_list()};
    case VAL_OBJECT:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_object()};
    case VAL_FUNCTION:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_function()};
    case VAL_BOUND_METHOD:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_bound_method()};
    case VAL_TYPE:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_type()};
    case VAL_NULL:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_null()};
    case VAL_UNDEFINED:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_undefined()};
    case VAL_PROMISE:
        return (Value){.type = VAL_TYPE, .cls = promise_namespace_type()};
    case VAL_INSTANCE:
        return (Value){.type = VAL_TYPE, .cls = value.instance->cls};
    default:
        return (Value){.type = VAL_TYPE, .cls = builtin_type_undefined()};
    }
}

bool builtin_type_is_namespace(Type *type)
{
    return type == builtin_type_number() || type == builtin_type_string() ||
           type == builtin_type_boolean() || type == builtin_type_list() ||
           type == builtin_type_object() || type == builtin_type_function() ||
           type == builtin_type_bound_method() || type == builtin_type_type() ||
           type == builtin_type_instance() || type == builtin_type_null() ||
           type == builtin_type_undefined();
}

bool builtin_type_is_number(Type *type)
{
    return type == builtin_type_number();
}

void builtin_types_register(Env *env)
{
    Value number_val = {.type = VAL_TYPE, .cls = builtin_type_number()};
    set_variable(env, "Number", number_val);

    Value string_val = {.type = VAL_TYPE, .cls = builtin_type_string()};
    set_variable(env, "String", string_val);

    Value bool_val = {.type = VAL_TYPE, .cls = builtin_type_boolean()};
    set_variable(env, "Boolean", bool_val);

    Value list_val = {.type = VAL_TYPE, .cls = builtin_type_list()};
    set_variable(env, "List", list_val);

    Value obj_val = {.type = VAL_TYPE, .cls = builtin_type_object()};
    set_variable(env, "Object", obj_val);

    Value fn_val = {.type = VAL_TYPE, .cls = builtin_type_function()};
    set_variable(env, "Function", fn_val);

    Value bound_val = {.type = VAL_TYPE, .cls = builtin_type_bound_method()};
    set_variable(env, "BoundMethod", bound_val);

    Value type_val = {.type = VAL_TYPE, .cls = builtin_type_type()};
    set_variable(env, "Type", type_val);

    Value instance_val = {.type = VAL_TYPE, .cls = builtin_type_instance()};
    set_variable(env, "Instance", instance_val);

    Value null_val = {.type = VAL_TYPE, .cls = builtin_type_null()};
    set_variable(env, "Null", null_val);

    Value undef_val = {.type = VAL_TYPE, .cls = builtin_type_undefined()};
    set_variable(env, "Undefined", undef_val);
}
