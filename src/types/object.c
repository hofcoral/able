#include <stdlib.h>
#include <string.h>

#include "types/object.h"
#include "types/value.h"

Object *object_create(void)
{
    Object *obj = malloc(sizeof(Object));
    if (!obj)
        return NULL;
    obj->count = 0;
    obj->capacity = 0;
    obj->pairs = NULL;
    return obj;
}

Object *clone_object(const Object *src)
{
    if (!src)
        return NULL;

    Object *copy = malloc(sizeof(Object));
    if (!copy)
        return NULL;

    copy->count = src->count;
    copy->capacity = src->capacity;
    copy->pairs = malloc(sizeof(KeyValuePair) * copy->capacity);
    if (!copy->pairs)
    {
        free(copy);
        return NULL;
    }

    for (int i = 0; i < src->count; ++i)
    {
        copy->pairs[i].key = strdup(src->pairs[i].key);
        copy->pairs[i].value = clone_value(&src->pairs[i].value);
    }

    return copy;
}

void free_object(Object *obj)
{
    if (!obj)
        return;

    for (int i = 0; i < obj->count; ++i)
    {
        free(obj->pairs[i].key);
        free_value(obj->pairs[i].value);
    }

    free(obj->pairs);
    free(obj);
}

// Optional: Get value for a key
Value object_get(Object *obj, const char *key)
{
    for (int i = 0; i < obj->count; ++i)
    {
        if (strcmp(obj->pairs[i].key, key) == 0)
        {
            return obj->pairs[i].value;
        }
    }

    Value v = {.type = VAL_NULL};
    return v;
}

// Optional: Insert or update key
void object_set(Object *obj, const char *key, Value val)
{
    for (int i = 0; i < obj->count; ++i)
    {
        if (strcmp(obj->pairs[i].key, key) == 0)
        {
            free_value(obj->pairs[i].value);
            obj->pairs[i].value = clone_value(&val);
            return;
        }
    }

    // Resize if needed
    if (obj->count >= obj->capacity)
    {
        obj->capacity = obj->capacity > 0 ? obj->capacity * 2 : 4;
        obj->pairs = realloc(obj->pairs, sizeof(KeyValuePair) * obj->capacity);
    }

    obj->pairs[obj->count].key = strdup(key);
    obj->pairs[obj->count].value = clone_value(&val);
    obj->count++;
}
