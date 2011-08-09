#ifndef __SEE_IO_H__
#define __SEE_IO_H__

#include "../as/ast.h"
#include "object.h"

void object_dump(object_t o);
expression_t expression_from_ast(heap_t heap, ast_node_t node);

#endif
