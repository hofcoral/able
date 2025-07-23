#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast/ast.h"
#include "types/value.h"

Value run_ast(ASTNode **nodes, int count);

#endif
