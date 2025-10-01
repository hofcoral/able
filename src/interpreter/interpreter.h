#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast/ast.h"
#include "types/value.h"
#include "types/env.h"
#include "types/function.h"
#include "interpreter/stack.h"

void interpreter_init();
void interpreter_cleanup();
void interpreter_set_env(Env *env);
void interpreter_pop_env();
Env *interpreter_current_env();
Value run_ast(ASTNode **nodes, int count);
Value interpreter_create_async_promise(Function *fn, Value *args, int arg_count, bool has_self, Value self, int line, int column);
Value interpreter_await(Value awaited, int line, int column);
Value interpreter_call_value(Value callee, Value *args, int arg_count, int line, int column);
Value interpreter_call_and_await(Value callee, Value *args, int arg_count, int line, int column);

#endif
