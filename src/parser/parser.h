#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "ast/ast.h"
#include "lexer/lexer.h"

/* entry-point */
ASTNode **parse_program(Lexer *lexer, int *out_count);
ASTNode *parse_object_literal();

/* utility */
void free_ast(ASTNode **nodes, int count);

#endif
