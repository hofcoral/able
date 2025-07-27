#ifndef STACK_H
#define STACK_H

#include "types/env.h"
#include "ast/ast.h"
#include <stdbool.h>

typedef struct CallFrame {
    Env *env;
    struct ASTNode **return_ptr; // optional future use
    bool returning;
} CallFrame;

typedef struct CallStack {
    CallFrame *frames;
    int size;
    int capacity;
} CallStack;

void stack_init(CallStack *stack);
void stack_free(CallStack *stack);
void push_frame(CallStack *stack, CallFrame frame);
void pop_frame(CallStack *stack);
CallFrame *current_frame(CallStack *stack);

#endif
