#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast/ast.h"
#include "types/value.h"
#include "types/env.h"

void interpreter_set_env(Env *env);
Value run_ast(ASTNode **nodes, int count);

#endif
