#include <stdlib.h>
#include <string.h>

#include "types/function.h"
#include "types/env.h"

void free_function(Function *fn)
{
    if (!fn)
        return;

    for (int i = 0; i < fn->param_count; ++i)
        free(fn->params[i]);
    free(fn->params);
    free(fn->name);
    if (fn->env)
        env_release(fn->env);
    if (fn->body)
    {
        for (int i = 0; i < fn->body_count; ++i)
            ; // body nodes freed elsewhere in AST cleanup
    }
    free(fn);
}
