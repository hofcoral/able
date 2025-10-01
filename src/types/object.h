#ifndef OBJECT_H
#define OBJECT_H

#include "value.h"

// ————— STRUCT FOR OBJECT PAIR ————— //
typedef struct
{
    char *key;
    Value value;
} KeyValuePair;

// ————— OBJECT STRUCT ————— //
typedef struct Object
{
    int count;
    int capacity;
    KeyValuePair *pairs;
} Object;

// ————— FUNCTIONS ————— //
Object *object_create(void);
Object *clone_object(const Object *src);
void free_object(Object *obj);
Value object_get(Object *obj, const char *key);           // Optional helper
void object_set(Object *obj, const char *key, Value val); // Optional helper

#endif
