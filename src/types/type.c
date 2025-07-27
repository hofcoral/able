#include <stdlib.h>
#include <string.h>
#include "types/type.h"

Type *type_create(const char *name, size_t size)
{
    Type *t = malloc(sizeof(Type));
    if (!t)
        return NULL;
    t->name = name ? strdup(name) : NULL;
    t->size = size;
    t->methods = NULL;
    t->parent = NULL;
    return t;
}

void type_free(Type *type)
{
    if (!type)
        return;
    free((char *)type->name);
    free(type);
}
