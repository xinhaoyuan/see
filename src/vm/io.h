#ifndef __SEE_IO_H__
#define __SEE_IO_H__

#include "../as/ast.h"
#include "../object.h"

void         object_dump(object_t o);
object_t     handle_from_ast(heap_t heap, ast_node_t node);
expression_t handle_expression_get(object_t handle);

#endif
