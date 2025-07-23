#include <stdio.h>
#include <stdlib.h>

#include "interpreter/resolve.h"
#include "types/object.h"
#include "types/value.h"
#include "ast/ast.h"
#include "types/variable.h"

Value resolve_attribute_chain(ASTNode *attr_node)
{
    Value base = get_variable(attr_node->object_name);
    if (base.type != VAL_OBJECT)
    {
        fprintf(stderr, "Error: '%s' is not an object\n", attr_node->object_name);
        exit(1);
    }

    for (int i = 0; i < attr_node->child_count; ++i)
    {
        attr_node = attr_node->children[i];
        base = object_get(base.obj, attr_node->attr_name);

        if (i < attr_node->child_count - 1 && base.type != VAL_OBJECT)
        {
            fprintf(stderr, "Error: intermediate '%s' is not an object\n", attr_node->attr_name);
            exit(1);
        }
    }

    return base;
}

void assign_attribute_chain(ASTNode *attr_node, Value val)
{
    Value base_val = get_variable(attr_node->object_name);
    if (base_val.type != VAL_OBJECT)
    {
        fprintf(stderr, "Error: '%s' is not an object\n", attr_node->object_name);
        exit(1);
    }

    Object *obj = base_val.obj;
    for (int i = 0; i < attr_node->child_count - 1; ++i)
    {
        ASTNode *seg = attr_node->children[i];
        Value next = object_get(obj, seg->attr_name);

        if (next.type == VAL_NULL)
        {
            Object *new_obj = malloc(sizeof(Object));
            new_obj->count = 0;
            new_obj->capacity = 0;
            new_obj->pairs = NULL;
            Value new_val = {.type = VAL_OBJECT, .obj = new_obj};
            object_set(obj, seg->attr_name, new_val);
            next = new_val;
        }
        else if (next.type != VAL_OBJECT)
        {
            fprintf(stderr, "Error: intermediate '%s' is not an object\n", seg->attr_name);
            exit(1);
        }

        obj = next.obj;
    }

    ASTNode *last = attr_node->children[attr_node->child_count - 1];
    object_set(obj, last->attr_name, val);
}
