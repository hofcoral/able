#include <stdlib.h>

#include "types/instance.h"

Instance *instance_create(Type *cls) {
    Instance *inst = malloc(sizeof(Instance));
    inst->cls = cls;
    inst->attributes = malloc(sizeof(Object));
    inst->attributes->count = 0;
    inst->attributes->capacity = 0;
    inst->attributes->pairs = NULL;
    return inst;
}

void instance_free(Instance *inst) {
    if (!inst)
        return;
    free_object(inst->attributes);
    free(inst);
}
