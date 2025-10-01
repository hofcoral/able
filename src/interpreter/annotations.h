#ifndef ANNOTATIONS_H
#define ANNOTATIONS_H

#include <stdbool.h>

#include "types/value.h"

typedef enum
{
    ANNOTATION_HANDLER_MODIFIER,
    ANNOTATION_HANDLER_DECORATOR
} AnnotationHandlerType;

void annotations_init(void);
void annotations_cleanup(void);
void annotations_register(const char *name, AnnotationHandlerType type, Value handler);
Value annotations_clone_handler(const char *name, AnnotationHandlerType type);
bool annotations_has_handler(const char *name, AnnotationHandlerType type);

#endif
