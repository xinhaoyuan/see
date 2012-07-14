#ifndef __SEE_IO_H__
#define __SEE_IO_H__

#include "../as/ast.h"
#include "../object.h"

typedef object_t(*external_constant_match_f)(const char *name, int len);

void         object_dump(object_t o, void *stream);
object_t     handle_from_ast(heap_t heap, ast_node_t node, external_constant_match_f cm);
expression_t handle_expression_get(object_t handle);

#endif
