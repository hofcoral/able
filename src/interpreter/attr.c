#include <stdlib.h>
#include <string.h>

#include "interpreter/attr.h"

static Value type_lookup(Type *t, const char *name)
{
    Value val = object_get(t->attributes, name);
    if (val.type != VAL_NULL)
        return val;
    for (int i = 0; i < t->base_count; ++i)
    {
        val = type_lookup(t->bases[i], name);
        if (val.type != VAL_NULL && val.type != VAL_UNDEFINED)
            return val;
    }
    Value undef = {.type = VAL_UNDEFINED};
    return undef;
}

Value value_get_attr(Value receiver, const char *name)
{
    if (receiver.type == VAL_INSTANCE)
    {
        Value attr = object_get(receiver.instance->attributes, name);
        if (attr.type != VAL_NULL)
        {
            if (attr.type == VAL_FUNCTION && attr.func->bind_on_access)
            {
                BoundMethod *bm = malloc(sizeof(BoundMethod));
                bm->self = receiver.instance;
                bm->func = attr.func;
                Value v = {.type = VAL_BOUND_METHOD, .bound = bm};
                return v;
            }
            return attr;
        }
        attr = type_lookup(receiver.instance->cls, name);
        if (attr.type != VAL_UNDEFINED && attr.type != VAL_NULL)
        {
            if (attr.type == VAL_FUNCTION && attr.func->bind_on_access)
            {
                BoundMethod *bm = malloc(sizeof(BoundMethod));
                bm->self = receiver.instance;
                bm->func = attr.func;
                Value v = {.type = VAL_BOUND_METHOD, .bound = bm};
                return v;
            }
            return attr;
        }
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }
    else if (receiver.type == VAL_TYPE)
    {
        Value attr = object_get(receiver.cls->attributes, name);
        if (attr.type == VAL_NULL)
        {
            Value undef = {.type = VAL_UNDEFINED};
            return undef;
        }
        return attr;
    }
    else if (receiver.type == VAL_FUNCTION)
    {
        if (!receiver.func->attributes)
        {
            Value undef = {.type = VAL_UNDEFINED};
            return undef;
        }
        Value attr = object_get(receiver.func->attributes, name);
        if (attr.type == VAL_NULL)
        {
            Value undef = {.type = VAL_UNDEFINED};
            return undef;
        }
        return attr;
    }
    else if (receiver.type == VAL_OBJECT)
    {
        return object_get(receiver.obj, name);
    }

    Value undef = {.type = VAL_UNDEFINED};
    return undef;
}

void value_set_attr(Value receiver, const char *name, Value val)
{
    if (receiver.type == VAL_INSTANCE)
    {
        object_set(receiver.instance->attributes, name, val);
        return;
    }
    if (receiver.type == VAL_TYPE)
    {
        object_set(receiver.cls->attributes, name, val);
        return;
    }
    if (receiver.type == VAL_FUNCTION)
    {
        if (!receiver.func->attributes)
            receiver.func->attributes = object_create();
        object_set(receiver.func->attributes, name, val);
        return;
    }
    if (receiver.type == VAL_OBJECT)
    {
        object_set(receiver.obj, name, val);
        return;
    }
}

