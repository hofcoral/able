#include <stdlib.h>
#include <string.h>

#include "types/type.h"

Type *type_create(const char *name) {
    Type *t = malloc(sizeof(Type));
    t->name = name ? strdup(name) : NULL;
    t->bases = NULL;
    t->base_count = 0;
    t->attributes = malloc(sizeof(Object));
    t->attributes->count = 0;
    t->attributes->capacity = 0;
    t->attributes->pairs = NULL;
    return t;
}

void type_set_bases(Type *t, Type **bases, int base_count) {
    t->bases = bases;
    t->base_count = base_count;
}

void type_free(Type *type) {
    if (!type)
        return;
    free(type->name);
    free(type->bases);
    free_object(type->attributes);
    free(type);
}
