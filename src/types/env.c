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
    env->count = 0;
    env->capacity = 0;
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

    for (int i = 0; i < env->count; ++i)
    {
        free(env->vars[i].name);
        free_value(env->vars[i].value);
    }

    free(env->vars);
    free(env);
}

static Variable *find_var(Env *env, const char *name)
{
    for (int i = 0; i < env->count; ++i)
    {
        if (strcmp(env->vars[i].name, name) == 0)
            return &env->vars[i];
    }
    return NULL;
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
    if (env->count == env->capacity)
    {
        env->capacity = env->capacity ? env->capacity * 2 : 4;
        env->vars = realloc(env->vars, sizeof(Variable) * env->capacity);
    }

    env->vars[env->count].name = strdup(name);
    env->vars[env->count].value = clone_value(&val);
    env->count++;
}

Value get_variable(Env *env, const char *name, int line)
{
    for (Env *e = env; e != NULL; e = e->parent)
    {
        Variable *var = find_var(e, name);
        if (var)
            return var->value;
    }

    log_script_error(line, "Runtime error: variable '%s' is not defined.", name);
    exit(1);
}
