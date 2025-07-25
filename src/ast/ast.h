#ifndef AST_H
#define AST_H

#include <stdbool.h>

#include "types/value.h"
#include "lexer/lexer.h"

typedef enum
{
    NODE_SET,
    NODE_VAR,
    NODE_FUNC_CALL,
    NODE_ATTR_ACCESS,
    NODE_LITERAL,
    NODE_RETURN,
    NODE_BINARY
} NodeType;

typedef struct ASTNode
{
    NodeType type;

    // Common
    struct ASTNode **children;
    int child_count;

    // SET
    char *set_name;
    struct ASTNode *set_attr; // destination attribute chain if assigning to attr

    // Attribute access
    char *object_name;
    char *attr_name;

    // Function call
    char *func_name;
    struct ASTNode *func_callee; // allow attribute based calls

    // Binary expression
    char binary_op;

    // Literal value (used for NODE_LITERAL)
    Value literal_value;

} ASTNode;

/* Helpers */
ASTNode *new_node(NodeType type);
void add_child(ASTNode *parent, ASTNode *child);

/* Cleanup */
void free_ast(ASTNode **nodes, int count);

#endif
