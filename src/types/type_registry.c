#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "types/type_registry.h"

#define MAX_TYPES 32

static Type *types[MAX_TYPES];
static int type_count = 0;

static void register_type(Type *t)
{
    if (type_count < MAX_TYPES)
        types[type_count++] = t;
}

void type_registry_init()
{
    type_count = 0;
    // Register built-in types
    register_type(type_create("undefined"));
    register_type(type_create("null"));
    register_type(type_create("bool"));
    register_type(type_create("number"));
    register_type(type_create("string"));
    register_type(type_create("object"));
    register_type(type_create("function"));
    register_type(type_create("list"));
}

void type_registry_cleanup()
{
    for (int i = 0; i < type_count; ++i)
    {
        type_free(types[i]);
        types[i] = NULL;
    }
    type_count = 0;
}

Type *type_registry_get(const char *name)
{
    for (int i = 0; i < type_count; ++i)
    {
        if (strcmp(types[i]->name, name) == 0)
            return types[i];
    }
    return NULL;
}
