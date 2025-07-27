#include <stdlib.h>
#include <string.h>

#include "types/env.h"
#include "types/object.h"
#include "types/value.h"
#include "utils/utils.h"

Env *current_env = NULL;

Env *env_create(Env *parent)
{
    Env *env = malloc(sizeof(Env));
    env->parent = parent;
    env->vars = NULL;
    env->ref_count = 1;
    return env;
}

void env_retain(Env *env)
{
    if (env)
        env->ref_count++;
}

void env_release(Env *env)
{
    if (!env)
        return;

    if (--env->ref_count > 0)
        return;

    Variable *cur, *tmp;
    HASH_ITER(hh, env->vars, cur, tmp)
    {
        HASH_DEL(env->vars, cur);
        free(cur->name);
        free_value(cur->value);
        free(cur);
    }

    free(env);
}

static Variable *find_var(Env *env, const char *name)
{
    Variable *var = NULL;
    HASH_FIND_STR(env->vars, name, var);
    return var;
}

void set_variable(Env *env, const char *name, Value val)
{
    // Search existing variable in chain
    for (Env *e = env; e != NULL; e = e->parent)
    {
        Variable *var = find_var(e, name);
        if (var)
        {
            free_value(var->value);
            var->value = clone_value(&val);
            return;
        }
    }

    // Add to current environment
    Variable *new_var = malloc(sizeof(Variable));
    new_var->name = strdup(name);
    new_var->value = clone_value(&val);
    HASH_ADD_KEYPTR(hh, env->vars, new_var->name, strlen(new_var->name), new_var);
}

Value get_variable(Env *env, const char *name, int line, int column)
{
    for (Env *e = env; e != NULL; e = e->parent)
    {
        Variable *var = find_var(e, name);
        if (var)
            return var->value;
    }

    log_script_error(line, column, "Runtime error: variable '%s' is not defined.", name);
    exit(1);
}
