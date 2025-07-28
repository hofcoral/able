#include <stdio.h>
#include <stdlib.h>

#include "interpreter/resolve.h"
#include "types/object.h"
#include "types/value.h"
#include "ast/ast.h"
#include "types/env.h"
#include "interpreter/interpreter.h"
#include "interpreter/attr.h"
#include "utils/utils.h"

static int is_container(Value v)
{
    return v.type == VAL_OBJECT || v.type == VAL_INSTANCE || v.type == VAL_TYPE;
}

Value resolve_attribute_chain(ASTNode *attr_node)
{
    Value base = get_variable(interpreter_current_env(),
                              attr_node->data.attr.object_name,
                              attr_node->line, attr_node->column);
    if (!is_container(base))
    {
        log_script_error(attr_node->line, attr_node->column,
                          "Error: '%s' is not an object",
                          attr_node->data.attr.object_name);
        exit(1);
    }

    for (int i = 0; i < attr_node->child_count; ++i)
    {
        ASTNode *seg = attr_node->children[i];
        base = value_get_attr(base, seg->data.attr.attr_name);
        if (i < attr_node->child_count - 1 && !is_container(base))
        {
            log_script_error(seg->line, seg->column,
                              "Error: intermediate '%s' is not an object",
                              seg->data.attr.attr_name);
            exit(1);
        }
    }

    return base;
}

void assign_attribute_chain(ASTNode *attr_node, Value val)
{
    Value base = get_variable(interpreter_current_env(),
                              attr_node->data.attr.object_name,
                              attr_node->line, attr_node->column);
    if (!is_container(base))
    {
        log_script_error(attr_node->line, attr_node->column,
                          "Error: '%s' is not an object",
                          attr_node->data.attr.object_name);
        exit(1);
    }

    for (int i = 0; i < attr_node->child_count - 1; ++i)
    {
        ASTNode *seg = attr_node->children[i];
        Value next = value_get_attr(base, seg->data.attr.attr_name);
        if (next.type == VAL_NULL || next.type == VAL_UNDEFINED)
        {
            Object *new_obj = malloc(sizeof(Object));
            new_obj->count = 0;
            new_obj->capacity = 0;
            new_obj->pairs = NULL;
            Value new_val = {.type = VAL_OBJECT, .obj = new_obj};
            value_set_attr(base, seg->data.attr.attr_name, new_val);
            next = new_val;
        }
        else if (!is_container(next))
        {
            log_script_error(seg->line, seg->column,
                              "Error: intermediate '%s' is not an object",
                              seg->data.attr.attr_name);
            exit(1);
        }
        base = next;
    }

    ASTNode *last = attr_node->children[attr_node->child_count - 1];
    value_set_attr(base, last->data.attr.attr_name, val);
}

