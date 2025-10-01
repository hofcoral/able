#ifndef FUNCTION_H
#define FUNCTION_H

#include "../ast/ast.h"
#include <stdbool.h>

struct Env;
struct Object;

typedef struct Function
{
    char *name;
    int param_count;
    char **params;
    ASTNode **body;
    int body_count;
    struct Env *env;
    struct Object *attributes;
    bool bind_on_access;
    bool is_async;
} Function;

#endif
