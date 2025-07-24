#include <stdlib.h>
#include <stdio.h>
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
    case VAL_FUNCTION:
        /* functions are freed as part of AST cleanup */
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
        printf("<function>");
        break;

    default:
        printf("undefined");
        break;
    }
}
