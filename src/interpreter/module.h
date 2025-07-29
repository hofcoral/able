#ifndef MODULE_H
#define MODULE_H

#include "types/env.h"
#include "types/value.h"

void module_system_init(Env *global_env, const char *exec_path);
void module_system_cleanup();
Value import_module_value(const char *name, int line, int column);
Value import_module_attr(const char *mod, const char *attr, int line, int column);

#endif
