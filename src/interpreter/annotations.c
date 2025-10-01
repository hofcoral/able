#include <stdlib.h>
#include <string.h>

#include "interpreter/annotations.h"
#include "utils/utils.h"
#include "uthash.h"

typedef struct AnnotationHandlerEntry
{
    char *name;
    Value handler;
    UT_hash_handle hh;
} AnnotationHandlerEntry;

static AnnotationHandlerEntry *modifier_handlers = NULL;
static AnnotationHandlerEntry *decorator_handlers = NULL;

static AnnotationHandlerEntry **select_table(AnnotationHandlerType type)
{
    return type == ANNOTATION_HANDLER_MODIFIER ? &modifier_handlers : &decorator_handlers;
}

void annotations_init(void)
{
    modifier_handlers = NULL;
    decorator_handlers = NULL;
}

void annotations_cleanup(void)
{
    AnnotationHandlerEntry *entry, *tmp;
    HASH_ITER(hh, modifier_handlers, entry, tmp)
    {
        HASH_DEL(modifier_handlers, entry);
        free(entry->name);
        free_value(entry->handler);
        free(entry);
    }
    HASH_ITER(hh, decorator_handlers, entry, tmp)
    {
        HASH_DEL(decorator_handlers, entry);
        free(entry->name);
        free_value(entry->handler);
        free(entry);
    }
}

void annotations_register(const char *name, AnnotationHandlerType type, Value handler)
{
    if (!name)
        return;
    AnnotationHandlerEntry **table = select_table(type);
    AnnotationHandlerEntry *entry = NULL;
    HASH_FIND_STR(*table, name, entry);
    if (!entry)
    {
        entry = malloc(sizeof(AnnotationHandlerEntry));
        entry->name = strdup(name);
        HASH_ADD_KEYPTR(hh, *table, entry->name, strlen(entry->name), entry);
    }
    else
    {
        free_value(entry->handler);
    }
    entry->handler = clone_value(&handler);
}

Value annotations_clone_handler(const char *name, AnnotationHandlerType type)
{
    AnnotationHandlerEntry **table = select_table(type);
    AnnotationHandlerEntry *entry = NULL;
    HASH_FIND_STR(*table, name, entry);
    if (!entry)
    {
        Value undef = {.type = VAL_UNDEFINED};
        return undef;
    }
    return clone_value(&entry->handler);
}

bool annotations_has_handler(const char *name, AnnotationHandlerType type)
{
    AnnotationHandlerEntry **table = select_table(type);
    AnnotationHandlerEntry *entry = NULL;
    HASH_FIND_STR(*table, name, entry);
    return entry != NULL;
}
