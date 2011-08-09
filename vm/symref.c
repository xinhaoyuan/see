#include <stdio.h>

#include "symref.h"
#include "scope.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

void
sematic_symref_analyse_internal(ast_node_t root, static_scope_t scope)
{
	switch (root->type)
	{

	case AST_SYMBOL:
	{
		as_symbol_t sym = (as_symbol_t)(root + 1);
		if (sym->type == SYMBOL_GENERAL)
		{
			root->priv = static_scope_find(sym->str, scope);
#if 0
			static_scope_ref_t ref = root->priv;
			if (ref == NULL)
				printf("%s ref to unknown\n", xstring_cstr(sym->str));
			else printf("%s ref to %d %d\n", xstring_cstr(sym->str), ref->lev_diff, ref->offset);
#endif
		}

		break;
	}

	case AST_APPLY:
	{
		ast_apply_t app = (ast_apply_t)(root + 1);
		sematic_symref_analyse_internal(app->func, scope);
		int i;
		for (i = 0; i != app->argc; ++ i)
		{
			sematic_symref_analyse_internal(app->args[i], scope);
		}

		break;
	}

	case AST_SET:
	{
		ast_set_t set = (ast_set_t)(root + 1);
		root->priv = static_scope_find(set->name, scope);
#if 0
		static_scope_ref_t ref = root->priv;
		if (ref == NULL)
			printf("set! %s ref to unknown\n", xstring_cstr(set->name));
		else printf("set! %s ref to %d %d\n", xstring_cstr(set->name), ref->lev_diff, ref->offset);
#endif

		sematic_symref_analyse_internal(set->value, scope);
		
		break;
	}

	case AST_COND:
	{
		ast_cond_t cond = (ast_cond_t)(root + 1);
		sematic_symref_analyse_internal(cond->c, scope);
		sematic_symref_analyse_internal(cond->t, scope);
		if (cond->e != NULL) sematic_symref_analyse_internal(cond->e, scope);
		
		break;
	}

	case AST_LAMBDA:
	{
		ast_lambda_t lambda = (ast_lambda_t)(root + 1);
		scope = static_scope_push(root, scope);
		sematic_symref_analyse_internal(lambda->proc, scope);
		scope = static_scope_pop(scope);
		
		break;
	}

	case AST_WITH:
	{
		ast_with_t with = (ast_with_t)(root + 1);
		scope = static_scope_push(root, scope);
		sematic_symref_analyse_internal(with->proc, scope);
		scope = static_scope_pop(scope);

		break;
	}

	case AST_PROC:
	{
		ast_proc_t proc = (ast_proc_t)(root + 1);
		int i;
		for (i = 0; i != proc->count; ++ i)
		{
			sematic_symref_analyse_internal(proc->nodes[i], scope);
		}
		
		break;
	}

	case AST_AND:
	{
		ast_and_t and = (ast_and_t)(root + 1);
		int i;
		for (i = 0; i != and->count; ++ i)
		{
			sematic_symref_analyse_internal(and->nodes[i], scope);
		}

		break;
	}

	case AST_OR:
	{
		ast_or_t or = (ast_or_t)(root + 1);
		int i;
		for (i = 0; i != or->count; ++ i)
		{
			sematic_symref_analyse_internal(or->nodes[i], scope);
		}

		break;
	}

	case AST_CALLCC:
	{
		ast_callcc_t c = (ast_callcc_t)(root + 1);
		sematic_symref_analyse_internal(c->node, scope);

		break;
	}
	
	}
}

void
sematic_symref_analyse(ast_node_t root)
{
	sematic_symref_analyse_internal(root, NULL);
}
