#ifndef ATTR_H
#define ATTR_H

#include "types/value.h"
#include "types/type.h"
#include "types/object.h"
#include "types/instance.h"
#include "types/function.h"

Value value_get_attr(Value receiver, const char *name);
void value_set_attr(Value receiver, const char *name, Value val);

#endif
