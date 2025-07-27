#ifndef ENV_H
#define ENV_H

#include "types/value.h"

#define MAX_VARS 128

typedef struct
{
    char *name;
    Value value;
} Variable;

typedef struct Env
{
    struct Env *parent;
    Variable *vars;
    int count;
    int capacity;
    int ref_count;
} Env;

Env *env_create(Env *parent);
void env_retain(Env *env);
void env_release(Env *env);

void set_variable(Env *env, const char *name, Value val);
Value get_variable(Env *env, const char *name, int line, int column);

extern Env *current_env;

#endif
