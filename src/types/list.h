#ifndef LIST_H
#define LIST_H

#include "value.h"

typedef struct List {
    int count;
    int capacity;
    Value *items;
} List;

List *clone_list(const List *src);
void free_list(List *list);
void list_append(List *list, Value val);
Value list_remove(List *list, int index);
Value list_get(List *list, int index);
void list_extend(List *list, const List *other);

#endif
