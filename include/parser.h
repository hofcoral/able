#ifndef PARSER_H
#define PARSER_H

#include "lexer.h" // token definitions

typedef enum
{
    NODE_SET,
    NODE_FUNC_CALL,
    NODE_VAR
} NodeType;

typedef struct ASTNode
{
    NodeType type;

    /* —— SET —— */
    char *set_name;  // identifier
    char *set_value; // value
    char *copy_from_var; // Copy from another var

    /* —— FUNC_CALL —— */
    char *func_name;
    struct ASTNode **args;
    int arg_count;

} ASTNode;

/* entry-point */
ASTNode **parse_program(Lexer *lexer, int *out_count);

/* utility */
void print_ast(ASTNode **nodes, int count, int indent);
void free_ast(ASTNode **nodes, int count);

#endif