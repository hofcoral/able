#ifndef RESOLVE_H
#define RESOLVE_H

#include "types/value.h"
#include "ast/ast.h"

Value resolve_attribute_chain(struct ASTNode *attr_node);
void assign_attribute_chain(struct ASTNode *attr_node, Value val);

#endif
