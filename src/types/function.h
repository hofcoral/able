#ifndef FUNCTION_H
#define FUNCTION_H

#include "../ast/ast.h"
#include <stdbool.h>

struct Env;

typedef struct Function
{
    char *name;
    int param_count;
    char **params;
    ASTNode **body;
    int body_count;
    struct Env *env;
    bool bind_on_access;
    bool is_async;
} Function;

#endif
