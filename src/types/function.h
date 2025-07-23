#ifndef FUNCTION_H
#define FUNCTION_H

#include "../ast/ast.h"

typedef struct Function
{
    int param_count;
    char **params;
    ASTNode **body;
    int body_count;
} Function;

void free_function(Function *fn);

#endif
