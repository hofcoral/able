#ifndef TYPE_H
#define TYPE_H

#include <stddef.h>

struct MethodTable; // forward declaration

typedef struct Type {
    const char *name;
    size_t size;
    struct MethodTable *methods;
    struct Type *parent;
} Type;

Type *type_create(const char *name, size_t size);
void type_free(Type *type);

#endif
