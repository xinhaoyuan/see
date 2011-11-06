#include "free.h"

#include <stdio.h>
#include <stdlib.h>

void
ast_free(ast_node_t node)
{
	if (node == NULL) return;
	int i;
	ast_node_t cur;
	ast_node_t next;

	switch (node->header.type)
	{
	case AST_GENERAL:
		cur = node->general.head;
		while (cur != NULL)
		{
			next = cur->header.next;
			ast_free(cur);

			if (next == node->general.head)
				cur = NULL;
			else cur = next;
		}
		break;

	case AST_SYMBOL:
		xstring_free(node->symbol.str);
		if (node->header.priv != NULL) free(node->header.priv);
		break;

	case AST_APPLY:
		ast_free(node->apply.func);
		for (i = 0; i < node->apply.argc; ++ i)
			ast_free(node->apply.args[i]);
		free(node->apply.args);
		break;

	case AST_SET:
		xstring_free(node->set.name);
		ast_free(node->set.value);
		if (node->header.priv != NULL) free(node->header.priv);
		break;

	case AST_COND:
		ast_free(node->cond.c);
		ast_free(node->cond.t);
		if (node->cond.e != NULL) ast_free(node->cond.e);
		break;

	case AST_LAMBDA:
		for (i = 0; i < node->lambda.argc; ++ i)
			xstring_free(node->lambda.args[i]);
		free(node->lambda.args);
		ast_free(node->lambda.proc);
		break;

	case AST_WITH:
		for (i = 0; i < node->with.varc; ++ i)
			xstring_free(node->with.vars[i]);
		free(node->with.vars);
		ast_free(node->with.proc);
		break;

	case AST_PROC:
		cur = node->proc.head;
		while (cur != NULL)
		{
			next = cur->header.next;
			ast_free(cur);

			if (next == node->proc.head)
				cur = NULL;
			else cur = next;
		}
		break;

	case AST_AND:
		cur = node->s_and.head;
		while (cur != NULL)
		{
			next = cur->header.next;
			ast_free(cur);

			if (next == node->s_and.head)
				cur = NULL;
			else cur = next;
		}
		break;

	case AST_OR:
		cur = node->s_or.head;
		while (cur != NULL)
		{
			next = cur->header.next;
			ast_free(cur);

			if (next == node->s_or.head)
				cur = NULL;
			else cur = next;
		}
		break;

	case AST_CALLCC:
		ast_free(node->callcc.node);
		break;

	default:
		fprintf(stderr, "ast_free: Unknown ast type: %d\n", node->header.type);
		break;
	}

	free(node);
}
