#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>

struct Object; // Forward declaration (to avoid circular include)
struct Function; // Forward declaration for functions
struct List;    // Forward declaration for lists
struct Type;
struct Instance;
struct Promise;

typedef struct BoundMethod {
    struct Instance *self;
    struct Function *func;
} BoundMethod;

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
    VAL_LIST,
    VAL_TYPE,
    VAL_INSTANCE,
    VAL_BOUND_METHOD,
    VAL_PROMISE,
    VAL_TYPE_COUNT
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
        struct List *list;
        struct Type *cls;
        struct Instance *instance;
        BoundMethod *bound;
        struct Promise *promise;
    };
} Value;

// ————— FUNCTIONS ————— //
Value clone_value(const Value *src);
void free_value(Value val);
void print_value(Value v, int indent); // For debugging
const char *value_type_name(ValueType type);

#endif
