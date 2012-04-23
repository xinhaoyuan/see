#include <config.h>
#include "symref.h"
#include "scope.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

void
sematic_symref_analyse_internal(ast_node_t root, static_scope_t scope)
{
	switch (root->header.type)
	{

	case AST_SYMBOL:
	{
		if (root->symbol.type == SYMBOL_GENERAL)
		{
			root->header.priv = static_scope_find(root->symbol.str, scope);
		}

		break;
	}

	case AST_APPLY:
	{
		sematic_symref_analyse_internal(root->apply.func, scope);
		int i;
		for (i = 0; i != root->apply.argc; ++ i)
		{
			sematic_symref_analyse_internal(root->apply.args[i], scope);
		}

		break;
	}

	case AST_SET:
	{
		root->header.priv = static_scope_find(root->set.name, scope);
		sematic_symref_analyse_internal(root->set.value, scope);
		
		break;
	}

	case AST_COND:
	{
		sematic_symref_analyse_internal(root->cond.c, scope);
		sematic_symref_analyse_internal(root->cond.t, scope);
		if (root->cond.e != NULL) sematic_symref_analyse_internal(root->cond.e, scope);
		
		break;
	}

	case AST_LAMBDA:
	{
		scope = static_scope_push(root, scope);
		sematic_symref_analyse_internal(root->lambda.proc, scope);
		scope = static_scope_pop(root, scope);
		
		break;
	}

	case AST_WITH:
	{
		scope = static_scope_push(root, scope);
		sematic_symref_analyse_internal(root->with.proc, scope);
		scope = static_scope_pop(root, scope);

		break;
	}

	case AST_PROC:
	{
		ast_node_t cur = root->proc.head;
		while (cur != NULL)
		{
			sematic_symref_analyse_internal(cur, scope);
			cur = cur->header.next;
			if (cur == root->proc.head) cur = NULL;
		}
		
		break;
	}

	case AST_AND:
	{
		ast_node_t cur = root->s_and.head;
		while (cur != NULL)
		{
			sematic_symref_analyse_internal(cur, scope);
			cur = cur->header.next;
			if (cur == root->s_and.head) cur = NULL;
		}

		break;
	}

	case AST_OR:
	{
		ast_node_t cur = root->s_or.head;
		while (cur != NULL)
		{
			sematic_symref_analyse_internal(cur, scope);
			cur = cur->header.next;
			if (cur == root->s_or.head) cur = NULL;
		}

		break;
	}

	case AST_CALLCC:
	{
		sematic_symref_analyse_internal(root->callcc.node, scope);

		break;
	}
	
	}
}

void
sematic_symref_analyse(ast_node_t root)
{
	sematic_symref_analyse_internal(root, NULL);
}
