#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"

void set_variable(const char *name, Value value);

Value get_variable(const char *name);

void run_ast(ASTNode **nodes, int count);

void free_vars();

#endif
