#include <stdlib.h>
#include "interpreter/stack.h"

void stack_init(CallStack *stack)
{
    stack->frames = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

void stack_free(CallStack *stack)
{
    free(stack->frames);
    stack->frames = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

void push_frame(CallStack *stack, CallFrame frame)
{
    if (stack->size >= stack->capacity)
    {
        stack->capacity = stack->capacity ? stack->capacity * 2 : 4;
        stack->frames = realloc(stack->frames, stack->capacity * sizeof(CallFrame));
    }
    stack->frames[stack->size++] = frame;
}

void pop_frame(CallStack *stack)
{
    if (stack->size > 0)
    {
        stack->size--;
    }
}

CallFrame *current_frame(CallStack *stack)
{
    if (stack->size == 0)
        return NULL;
    return &stack->frames[stack->size - 1];
}
