#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast/ast.h"
#include "types/value.h"
#include "types/env.h"
#include "interpreter/stack.h"

void interpreter_init();
void interpreter_cleanup();
void interpreter_set_env(Env *env);
void interpreter_pop_env();
Env *interpreter_current_env();
Value run_ast(ASTNode **nodes, int count);

#endif
