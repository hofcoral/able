#ifndef INSTANCE_H
#define INSTANCE_H

#include "types/type.h"
#include "types/object.h"

typedef struct Instance {
    int ref_count;
    Type *cls;
    Object *attributes;
} Instance;

Instance *instance_create(Type *cls);
void instance_retain(Instance *inst);
void instance_release(Instance *inst);

#endif
