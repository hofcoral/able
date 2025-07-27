#ifndef ENV_H
#define ENV_H

#include "types/value.h"

#include "uthash.h"

typedef struct Variable
{
    char *name;           // key
    Value value;          // stored value
    UT_hash_handle hh;    // uthash handle
} Variable;

typedef struct Env
{
    struct Env *parent;
    Variable *vars;       // hash table of variables
    int ref_count;
} Env;

Env *env_create(Env *parent);
void env_retain(Env *env);
void env_release(Env *env);

void set_variable(Env *env, const char *name, Value val);
Value get_variable(Env *env, const char *name, int line, int column);

#endif
