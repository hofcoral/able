#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "types/value.h"
#include "types/object.h"
#include "types/function.h"
#include "types/list.h"
#include "types/instance.h"

static const char *TYPE_NAMES[VAL_TYPE_COUNT] = {
    "UNDEFINED",
    "NULL",
    "BOOLEAN",
    "NUMBER",
    "STRING",
    "OBJECT",
    "FUNCTION",
    "LIST",
    "TYPE",
    "INSTANCE",
    "BOUND_METHOD"
};

const char *value_type_name(ValueType type)
{
    if (type < 0 || type >= VAL_TYPE_COUNT)
        return "UNKNOWN";
    return TYPE_NAMES[type];
}

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
    case VAL_LIST:
        free_list(v.list);
        break;
    case VAL_FUNCTION:
        /* functions are freed as part of AST cleanup */
        break;
    case VAL_TYPE:
        break;
    case VAL_INSTANCE:
        instance_release(v.instance);
        break;
    case VAL_BOUND_METHOD:
        free(v.bound);
        break;
    case VAL_BOOL:
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
    case VAL_FUNCTION:
        copy.func = src->func;
        break;
    case VAL_LIST:
        copy.list = clone_list(src->list);
        break;
    case VAL_TYPE:
        copy.cls = src->cls;
        break;
    case VAL_INSTANCE:
        copy.instance = src->instance;
        instance_retain(copy.instance);
        break;
    case VAL_BOUND_METHOD:
        if (src->bound) {
            BoundMethod *bm = malloc(sizeof(BoundMethod));
            bm->self = src->bound->self;
            bm->func = src->bound->func;
            copy.bound = bm;
        } else {
            copy.bound = NULL;
        }
        break;
    case VAL_BOOL:
        copy.boolean = src->boolean;
        break;
    case VAL_NULL:
    case VAL_UNDEFINED:
    default:
        copy.type = src->type;
        break;
    }

    return copy;
}

void print_value(Value v, int indent)
{
    switch (v.type)
    {
    case VAL_STRING:
        printf("%s", v.str);
        break;

    case VAL_NUMBER:
        if (fabs(v.num - (long long)v.num) < 1e-9)
            printf("%lld", (long long)v.num);
        else
            printf("%f", v.num);
        break;

    case VAL_BOOL:
        printf(v.boolean ? "true" : "false");
        break;

    case VAL_OBJECT:
        if (!v.obj || v.obj->count == 0)
        {
            printf("{}");
            break;
        }

        printf("{\n");

        for (int i = 0; i < v.obj->count; i++)
        {
            // Indent
            for (int j = 0; j < indent + 2; j++)
                printf(" ");

            printf("%s: ", v.obj->pairs[i].key);
            print_value(v.obj->pairs[i].value, indent + 2);

            if (i < v.obj->count - 1)
                printf(",");

            printf("\n");
        }

        for (int j = 0; j < indent; j++)
            printf(" ");
        printf("}\n");
        break;

    case VAL_FUNCTION:
        if (v.func)
        {
            const char *fname = v.func->name ? v.func->name : "anonymous";
            printf("<function: %s at %p>", fname, (void *)v.func);
        }
        else
        {
            printf("<function: NULL>");
        }
        break;
    case VAL_TYPE:
        printf("<type %s>", v.cls ? v.cls->name : "unknown");
        break;
    case VAL_INSTANCE:
        printf("<instance of %s at %p>",
               v.instance && v.instance->cls ? v.instance->cls->name : "?",
               (void *)v.instance);
        break;
    case VAL_BOUND_METHOD:
        printf("<bound method>");
        break;
    case VAL_LIST:
        printf("[");
        for (int i = 0; i < v.list->count; ++i)
        {
            print_value(v.list->items[i], indent);
            if (i < v.list->count - 1)
                printf(", ");
        }
        printf("]");
        break;

    default:
        printf("undefined");
        break;
    }
}
