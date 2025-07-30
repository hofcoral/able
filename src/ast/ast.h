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
    NODE_METHOD_DEF,
    NODE_FOR,
    NODE_WHILE,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_IMPORT_MODULE,
    NODE_IMPORT_NAMES,
    NODE_POSTFIX_INC,
    NODE_UNARY
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
    OP_GTE,
    OP_AND,
    OP_OR
} BinaryOp;

typedef enum
{
    UNARY_NOT
} UnaryOp;

typedef struct ASTNode
{
    NodeType type;

    // Common
    int line, column;
    struct ASTNode **children;
    int child_count;
    bool is_static;

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
            UnaryOp op;
        } unary;

        struct
        {
            char *class_name;
            char **base_names;
            int base_count;
        } cls;

        struct
        {
            char *method_name;
            char **params;
            int param_count;
        } method;

        struct
        {
            Value literal_value;
        } lit;

        struct
        {
            char *loop_var;
        } loop;

        struct
        {
            char *module_name;
        } import_module;

        struct
        {
            char *module_name;
            char **names;
            int name_count;
        } import_names;

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
ASTNode *new_import_module_node(char *module_name, int line, int column);
ASTNode *new_import_names_node(char *module_name, char **names, int name_count,
                               int line, int column);
ASTNode *new_postfix_inc_node(struct ASTNode *target);
ASTNode *new_unary_node(UnaryOp op, struct ASTNode *expr, int line, int column);
void add_child(ASTNode *parent, ASTNode *child);

/* Cleanup */
void free_ast(ASTNode **nodes, int count);

#endif
