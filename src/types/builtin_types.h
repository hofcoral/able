#ifndef BUILTIN_TYPES_H
#define BUILTIN_TYPES_H

#include <stdbool.h>

#include "types/type.h"
#include "types/value.h"

struct Env;

Type *builtin_type_number(void);
Type *builtin_type_string(void);
Type *builtin_type_boolean(void);
Type *builtin_type_list(void);
Type *builtin_type_object(void);
Type *builtin_type_function(void);
Type *builtin_type_bound_method(void);
Type *builtin_type_type(void);
Type *builtin_type_instance(void);
Type *builtin_type_null(void);
Type *builtin_type_undefined(void);

Value builtin_type_value_for(Value value);
bool value_is_type(Value value, Type *type);
bool type_is_subclass(Type *type, Type *base);

bool builtin_type_is_namespace(Type *type);
bool builtin_type_is_number(Type *type);

void builtin_types_register(struct Env *env);

#endif
