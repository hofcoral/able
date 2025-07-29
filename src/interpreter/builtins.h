#ifndef BUILTINS_H
#define BUILTINS_H

#include "types/env.h"

void builtins_register(Env *global_env, const char *file_path);

#endif
