#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "types/value.h"
#include "lexer/lexer.h"

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
    char *set_name;      // Target variable
    Value literal_value; // Direct value (string, number, object)
    char *copy_from_var; // Name of variable to copy from
    bool is_copy;

    /* —— FUNC_CALL —— */
    char *func_name;
    struct ASTNode **args;
    int arg_count;

} ASTNode;

/* Cleanup */
void free_ast(ASTNode **nodes, int count);

#endif
