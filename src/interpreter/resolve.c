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
