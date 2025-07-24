#ifndef FUNCTION_H
#define FUNCTION_H

#include "../ast/ast.h"

struct Env;

typedef struct Function
{
    char *name;
    int param_count;
    char **params;
    ASTNode **body;
    int body_count;
    struct Env *env;
} Function;

void free_function(Function *fn);

#endif
