#include <config.h>
#include "scope.h"


scope_ref_t
static_scope_find(xstring_t name, static_scope_t scope)
{
	int lev_diff = 0;
	int i;
	while (scope != NULL)
	{
		ast_node_t node = scope->node;
		if (node->header.type == AST_LAMBDA)
		{
			for (i = 0; i != node->lambda.argc; ++ i)
			{
				if (xstring_equal(node->lambda.args[i], name))
				{
					scope_ref_t result = (scope_ref_t)SEE_MALLOC(sizeof(scope_ref_s));
					result->parent_level = lev_diff;
					result->offset = i;

					return result;
				}
			}
		}
		else if (node->header.type == AST_WITH)
		{
			for (i = 0; i != node->with.varc; ++ i)
			{
				if (xstring_equal(node->with.vars[i], name))
				{
					scope_ref_t result = (scope_ref_t)SEE_MALLOC(sizeof(scope_ref_s));
					result->parent_level = lev_diff;
					result->offset = i;

					return result;
				}
			}
		}

		++ lev_diff;
		scope = scope->parent;
	}

	return NULL;
}

static_scope_t
static_scope_push(ast_node_t node, static_scope_t scope)
{
	static_scope_t result = (static_scope_t)SEE_MALLOC(sizeof(struct static_scope_s));
	result->node = node;
	result->dist = NULL;
	result->parent = scope;

	node->header.priv = result;
	return result;
}

static_scope_t
static_scope_pop(ast_node_t node, static_scope_t scope)
{
	static_scope_t r = scope->parent;
	SEE_FREE(scope);
	node->header.priv = NULL;
	
	return r;
}
