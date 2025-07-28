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
    NODE_BINARY,
    NODE_IF,
    NODE_BLOCK
} NodeType;

typedef enum
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_STRICT_EQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE
} BinaryOp;

typedef struct ASTNode
{
    NodeType type;

    // Common
    struct ASTNode **children;
    int child_count;

    union
    {
        struct
        {
            char *set_name;
            struct ASTNode *set_attr; // destination attribute chain if assigning to attr
        } set;
        struct
        {
            char *var_name;
        } var;
        struct
        {
            char *object_name;
            char *attr_name;
        } attr;
        struct
        {
            char *func_name;
            struct ASTNode *func_callee; // allow attribute based calls
        } call;
        struct
        {
            BinaryOp op;
        } binary;
        Value literal;
    } data;

    int line;
    int column;

} ASTNode;

/* Helpers */
ASTNode *new_node(NodeType type, int line, int column);
void add_child(ASTNode *parent, ASTNode *child);

ASTNode *new_set_node(char *name, ASTNode *attr, int line, int column);
ASTNode *new_var_node(char *name, int line, int column);
ASTNode *new_attr_node(char *object, char *attr, int line, int column);
ASTNode *new_func_call_node(ASTNode *callee, char *func_name, int line, int column);
ASTNode *new_literal_node(Value value, int line, int column);
ASTNode *new_binary_node(BinaryOp op, int line, int column);

/* Cleanup */
void free_ast(ASTNode **nodes, int count);

#endif
