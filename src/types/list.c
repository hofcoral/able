#include <stdlib.h>
#include <string.h>

#include "types/list.h"

List *clone_list(const List *src)
{
    if (!src)
        return NULL;
    List *copy = malloc(sizeof(List));
    if (!copy)
        return NULL;
    copy->count = src->count;
    copy->capacity = src->capacity;
    copy->items = malloc(sizeof(Value) * copy->capacity);
    for (int i = 0; i < src->count; ++i)
        copy->items[i] = clone_value(&src->items[i]);
    return copy;
}

void free_list(List *list)
{
    if (!list)
        return;
    for (int i = 0; i < list->count; ++i)
        free_value(list->items[i]);
    free(list->items);
    free(list);
}

static void ensure_capacity(List *list, int cap)
{
    if (list->capacity >= cap)
        return;
    list->capacity = list->capacity > 0 ? list->capacity * 2 : 4;
    if (list->capacity < cap)
        list->capacity = cap;
    list->items = realloc(list->items, sizeof(Value) * list->capacity);
}

void list_append(List *list, Value val)
{
    ensure_capacity(list, list->count + 1);
    list->items[list->count++] = clone_value(&val);
}

Value list_remove(List *list, int index)
{
    Value undef = {.type = VAL_UNDEFINED};
    if (index < 0 || index >= list->count)
        return undef;
    Value removed = list->items[index];
    for (int i = index; i < list->count - 1; ++i)
        list->items[i] = list->items[i + 1];
    list->count--;
    return removed;
}

Value list_get(List *list, int index)
{
    Value undef = {.type = VAL_UNDEFINED};
    if (index < 0 || index >= list->count)
        return undef;
    return list->items[index];
}

void list_extend(List *list, const List *other)
{
    ensure_capacity(list, list->count + other->count);
    for (int i = 0; i < other->count; ++i)
        list->items[list->count++] = clone_value(&other->items[i]);
}
