#ifndef TYPE_REGISTRY_H
#define TYPE_REGISTRY_H

#include "types/type.h"

void type_registry_init();
void type_registry_cleanup();
Type *type_registry_get(const char *name);

#endif
