#ifndef __SEE_SCOPE_H__
#define __SEE_SCOPE_H__

#include "../object.h"

#include "../lib/xstring.h"
#include "../as/ast.h"

struct static_scope_s
{
	ast_node_t node;
	void      *dist;
	struct static_scope_s *parent;
};

typedef struct static_scope_s *static_scope_t;


scope_ref_t    static_scope_find(xstring_t name, static_scope_t scope);
static_scope_t static_scope_push(ast_node_t node, static_scope_t scope);
static_scope_t static_scope_pop(ast_node_t node, static_scope_t scope);

#endif
