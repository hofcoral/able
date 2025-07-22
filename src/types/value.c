#include <stdlib.h>
#include <string.h>

#include "types/value.h"
#include "types/object.h"

void free_value(Value v)
{
    switch (v.type)
    {
    case VAL_STRING:
        free(v.str);
        break;
    case VAL_OBJECT:
        free_object(v.obj);
        break;
    default:
        // VAL_NUMBER, VAL_NULL, VAL_UNDEFINED don't need manual freeing
        break;
    }
}

Value clone_value(const Value *src)
{
    Value copy;
    copy.type = src->type;

    switch (src->type)
    {
    case VAL_STRING:
        copy.str = src->str ? strdup(src->str) : NULL;
        break;
    case VAL_NUMBER:
        copy.num = src->num;
        break;
    case VAL_OBJECT:
        copy.obj = clone_object(src->obj);
        break;
    case VAL_NULL:
    case VAL_UNDEFINED:
    default:
        copy.type = src->type;
        break;
    }

    return copy;
}
