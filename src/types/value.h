#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>

struct Object; // Forward declaration (to avoid circular include)
struct Function; // Forward declaration for functions

// ————— ENUM FOR VALUE TYPES ————— //
typedef enum
{
    VAL_UNDEFINED,
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_OBJECT,
    VAL_FUNCTION,
} ValueType;

// ————— VALUE STRUCT ————— //
typedef struct Value
{
    ValueType type;
    union
    {
        bool boolean;
        double num;
        char *str;
        struct Object *obj;
        struct Function *func;
    };
} Value;

// ————— FUNCTIONS ————— //
Value clone_value(const Value *src);
void free_value(Value val);
void print_value(Value v, int indent); // For debugging

#endif
