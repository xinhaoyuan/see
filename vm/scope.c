#include <stdlib.h>

#include "scope.h"

scope_ref_t
static_scope_find(xstring_t name, static_scope_t scope)
{
	int lev_diff = 0;
	int i;
	while (scope != NULL)
	{
		ast_node_t node = scope->node;
		if (node->type == AST_LAMBDA)
		{
			ast_lambda_t l = (ast_lambda_t)(node + 1);
			for (i = 0; i != l->argc; ++ i)
			{
				if (xstring_equal(l->args[i], name))
				{
					scope_ref_t result = (scope_ref_t)malloc(sizeof(scope_ref_s));
					result->parent_level = lev_diff;
					result->offset = i;

					return result;
				}
			}
		}
		else if (node->type == AST_WITH)
		{
			ast_with_t w = (ast_with_t)(node + 1);
			for (i = 0; i != w->varc; ++ i)
			{
				if (xstring_equal(w->vars[i], name))
				{
					scope_ref_t result = (scope_ref_t)malloc(sizeof(scope_ref_s));
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
	static_scope_t result = (static_scope_t)malloc(sizeof(struct static_scope_s));
	result->node = node;
	result->dist = NULL;
	result->parent = scope;

	node->priv = result;
	return result;
}

static_scope_t
static_scope_pop(static_scope_t scope)
{
	static_scope_t r = scope->parent;
	free(scope);
	
	return r;
}
