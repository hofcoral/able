#ifndef TYPE_H
#define TYPE_H

#include "types/object.h"

typedef struct Type {
    char *name;
    struct Type **bases;
    int base_count;
    Object *attributes;
} Type;

Type *type_create(const char *name);
void type_set_bases(Type *t, Type **bases, int base_count);
void type_free(Type *type);

#endif
