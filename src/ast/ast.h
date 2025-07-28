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
    NODE_BLOCK,
    NODE_CLASS_DEF,
    NODE_METHOD_DEF
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
    int line, column;
    struct ASTNode **children;
    int child_count;

    // Node-specific data
    union
    {
        struct
        {
            char *set_name;
            struct ASTNode *set_attr;
        } set;

        struct
        {
            char *object_name;
            char *attr_name;
        } attr;

        struct
        {
            char *func_name;
            struct ASTNode *func_callee;
        } call;

        struct
        {
            BinaryOp op;
        } binary;

        struct
        {
            Value literal_value;
        } lit;

        /* (add new kinds here—LIST_LITERAL, CLASS_DEF, FOR_LOOP, etc.) */
    } data;

} ASTNode;

/* Helpers */
ASTNode *new_node(NodeType type, int line, int column);
ASTNode *new_var_node(char *name, int line, int column);
ASTNode *new_attr_access_node(char *object_name, char *attr_name,
                              int line, int column);
ASTNode *new_set_node(char *name, struct ASTNode *attr, int line, int column);
ASTNode *new_func_call_node(struct ASTNode *callee);
void add_child(ASTNode *parent, ASTNode *child);

/* Cleanup */
void free_ast(ASTNode **nodes, int count);

#endif
