#ifndef INSTANCE_H
#define INSTANCE_H

#include "types/type.h"
#include "types/object.h"

typedef struct Instance {
    Type *cls;
    Object *attributes;
} Instance;

Instance *instance_create(Type *cls);
void instance_free(Instance *inst);

#endif
