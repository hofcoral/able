#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"

typedef enum
{
    VAL_STRING,
    VAL_NUMBER
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        char *str;
        double num;
    };
} Value;

void set_variable(const char *name, Value value);

Value get_variable(const char *name);

void run_ast(ASTNode **nodes, int count);

void free_vars();

#endif