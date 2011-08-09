#include <stdio.h>
#include <stdlib.h>

#include "io.h"

void
object_dump(object_t o)
{
	unsigned int suffix = encode_suffix(o);
	switch (suffix)
	{
	case ENCODE_SUFFIX_OBJECT:
		if (o == NULL)
		{
			printf(" (NULL)");
		}
		else
		{
			switch (o->gc.type)
			{
			case OBJECT_STRING:
				printf(" \"%s\"", xstring_cstr(o->string));
				break;

			case OBJECT_PAIR:
				printf(" (");
				object_dump(SLOT_GET(o->pair.slot_car));
				printf(" .");
				object_dump(SLOT_GET(o->pair.slot_cdr));
				printf(" )");
				break;

			case OBJECT_VECTOR:
			{
				printf(" [");
				int i;
				for (i = 0; i != o->vector.length; ++ i)
				{
					object_dump(SLOT_GET(o->vector.slot_entry[i]));
				}
				printf(" ]");
				break;
			}

			case OBJECT_ENVIRONMENT:
				printf(" (ENVIRONMENT)");
				break;

			case OBJECT_CLOSURE:
				printf(" (CLOSURE)");
				break;

			case OBJECT_CONTINUATION:
				printf(" (CONTINUATION)");
				break;

			case OBJECT_EXECUTION:
				printf(" (EXECUTION)");
				break;

			case OBJECT_EXTERNAL:
				printf(" (EXTERNAL)");
				break;

			default:
				printf(" (UNKNOWN)");
				break;
			}
		}
		break;

	case ENCODE_SUFFIX_INT:
		printf(" %d", int_unbox(o));
		break;

	case ENCODE_SUFFIX_SYMBOL:
		printf(" (SYMBOL)");
		break;
		
	case ENCODE_SUFFIX_BOXED:
		printf(" (BOXED:%08x)", external_unbox(o));
		break;
	}
}

static void
scan_ast(ast_node_t node, unsigned int *exps_count, unsigned *objs_count)
{
	++ (*exps_count);
	
	switch (node->type)
	{
	case AST_SYMBOL:
	{
		as_symbol_t s = (as_symbol_t)(node + 1);
		if (s->type == SYMBOL_STRING)
			++ (*objs_count);
		
		break;
	}

	case AST_LAMBDA:
	{
		ast_lambda_t l = (ast_lambda_t)(node + 1);
		scan_ast(l->proc, exps_count, objs_count);
		break;
	}

	case AST_WITH:
	{
		ast_with_t w = (ast_with_t)(node + 1);
		scan_ast(w->proc, exps_count, objs_count);
		break;
	}


	case AST_PROC:
	{
		ast_proc_t p = (ast_proc_t)(node + 1);
		int i;
		
		for (i = 0; i < p->count; ++ i)
			scan_ast(p->nodes[i], exps_count, objs_count);

		break;
	}

	case AST_AND:
	{
		ast_and_t p = (ast_and_t)(node + 1);
		int i;
		
		for (i = 0; i < p->count; ++ i)
			scan_ast(p->nodes[i], exps_count, objs_count);

		break;
	}

	case AST_OR:
	{
		ast_or_t p = (ast_or_t)(node + 1);
		int i;
		
		for (i = 0; i < p->count; ++ i)
			scan_ast(p->nodes[i], exps_count, objs_count);

		break;
	}


	case AST_APPLY:
	{
		ast_apply_t apply = (ast_apply_t)(node + 1);
		int i;
		
		scan_ast(apply->func, exps_count, objs_count);
		for (i = 0; i < apply->argc; ++ i)
			scan_ast(apply->args[i], exps_count, objs_count);
		
		break;
	}

	case AST_COND:
	{
		ast_cond_t _cond = (ast_cond_t)(node + 1);
		scan_ast(_cond->c, exps_count, objs_count);
		scan_ast(_cond->t, exps_count, objs_count);
		if (_cond->e != NULL)
			scan_ast(_cond->e, exps_count, objs_count);

		break;
	}
		
	case AST_SET:
	{
		ast_set_t s = (ast_set_t)(node + 1);
		scan_ast(s->value, exps_count, objs_count);
		break;
	}

	case AST_CALLCC:
	{
		ast_callcc_t s = (ast_callcc_t)(node + 1);
		scan_ast(s->node, exps_count, objs_count);
		break;
	}

	default:
	{
		break;
	}
		
	}
}

typedef struct exp_handle_priv_s
{
	unsigned int exps_count;
	unsigned int objs_count;

	expression_t exps;
	object_t    *objs;
} *exp_handle_priv_t;

static void
handle_free(object_t object)
{
	exp_handle_priv_t priv = (exp_handle_priv_t)object->external.priv;
	free(priv->exps);
	free(priv->objs);
	free(priv);
}

static void
handle_enumerate(object_t object, void *q, void(*enqueue)(void *, object_t))
{
	exp_handle_priv_t priv = (exp_handle_priv_t)object->external.priv;
	int i;
	for (i = 0; i != priv->objs_count; ++ i)
	{
		enqueue(q, priv->objs[i]);
	}
}

static expression_t 
expression_from_ast_internal(heap_t heap, ast_node_t node, object_t handle, exp_handle_priv_t priv)
{
	expression_t result = priv->exps + (priv->exps_count ++);
	result->handle = handle;
	
	switch (node->type)
	{
	case AST_SYMBOL:
	{
		int type = ((as_symbol_t)(node + 1))->type;
		switch (type)
		{
		case SYMBOL_NULL:
			result->type = EXP_TYPE_VALUE;
			result->value = NULL;
			break;
				
		case SYMBOL_GENERAL:
			result->type = EXP_TYPE_REF;
			if (node->priv == NULL)
			{
				result->ref.parent_level = -1;
			}
			else
			{
				result->ref  = *(scope_ref_t)node->priv;
				free(node->priv);
			}
			
			break;

		case SYMBOL_NUMERIC:
		{
			int v;
			sscanf(xstring_cstr(((as_symbol_t)(node + 1))->str), "%d", &v);

			result->type = EXP_TYPE_VALUE;
			result->value = int_box(v);
			break;
		}

		case SYMBOL_STRING:
			result->type = EXP_TYPE_VALUE;
			result->value = heap_object_new(heap);
			result->value->string = ((as_symbol_t)(node + 1))->str;
			result->value->gc.type = OBJECT_STRING;

			priv->objs[priv->objs_count ++] = result->value;
			
			break;
		}

		break;
	}

	case AST_LAMBDA:
	{
		ast_lambda_t l = (ast_lambda_t)(node + 1);
		result->type = EXP_TYPE_CLOSURE;
		result->closure.argc = l->tail_list ? l->argc : l->argc + 1;
		/* XXX */
		result->closure.inherit = 1;
		
		result->closure.child = expression_from_ast_internal(heap, l->proc, handle, priv);		
		result->closure.child->parent = result;
		result->closure.child->next = NULL;
		
		break;
	}

	case AST_WITH:
	{
		ast_with_t w = (ast_with_t)(node + 1);
		result->type = EXP_TYPE_WITH;
		result->with.varc = w->varc;
		result->with.inherit = 1;
		
		result->with.child = expression_from_ast_internal(heap, w->proc, handle, priv);
		result->with.child->parent = result;
		result->with.child->next = NULL;
		break;
	}


	case AST_PROC:
	{
		ast_proc_t p = (ast_proc_t)(node + 1);
		int i;
		
		result->type = EXP_TYPE_PROC;
		expression_t e = result->proc.child =
			expression_from_ast_internal(heap, p->nodes[0], handle, priv);
		for (i = 1; i < p->count; ++ i)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, p->nodes[i], handle, priv);
			e = e->next;
		}
		e->parent = result;
		e->next = NULL;

		break;
	}

	case AST_AND:
	{
		ast_and_t p = (ast_and_t)(node + 1);
		int i;
		
		result->type = EXP_TYPE_AND;
		expression_t e = result->and.child =
			expression_from_ast_internal(heap, p->nodes[0], handle, priv);
		for (i = 1; i < p->count; ++ i)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, p->nodes[i], handle, priv);
			e = e->next;
		}
		e->parent = result;
		e->next = NULL;

		break;
	}

	case AST_OR:
	{
		ast_or_t p = (ast_or_t)(node + 1);
		int i;
		
		result->type = EXP_TYPE_OR;
		expression_t e = result->and.child =
			expression_from_ast_internal(heap, p->nodes[0], handle, priv);
		for (i = 1; i < p->count; ++ i)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, p->nodes[i], handle, priv);
			e = e->next;
		}
		e->parent = result;
		e->next = NULL;

		break;
	}


	case AST_APPLY:
	{
		ast_apply_t apply = (ast_apply_t)(node + 1);
		int i;
		
		result->type = EXP_TYPE_APPLY;
		result->apply.argc = apply->argc;
		result->apply.tail = apply->tail;
		expression_t e = result->apply.child = expression_from_ast_internal(heap, apply->func, handle, priv);
		for (i = 0; i < apply->argc; ++ i)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, apply->args[i], handle, priv);
			e = e->next;
		}
		e->parent = result;
		e->next = NULL;
		
		break;
	}

	case AST_COND:
	{
		ast_cond_t cond = (ast_cond_t)(node + 1);
		result->type = EXP_TYPE_COND;
		
		result->cond.cd = expression_from_ast_internal(heap, cond->c, handle, priv);
		result->cond.cd->parent = result;
		result->cond.cd->next = NULL;
		
		result->cond.th = expression_from_ast_internal(heap, cond->t, handle, priv);
		result->cond.th->parent = result;
		result->cond.th->next = NULL;

		if (cond->e != NULL)
		{
			result->cond.el = expression_from_ast_internal(heap, cond->e, handle, priv);
			result->cond.el->parent = result;
			result->cond.el->next = NULL;
		}
		else result->cond.el = NULL;

		break;
	}
		
	case AST_SET:
	{
		ast_set_t s = (ast_set_t)(node + 1);
		result->type = EXP_TYPE_SET;
		result->set.exp = expression_from_ast_internal(heap, s->value, handle, priv);
		result->set.exp->parent = result;
		result->set.exp->next = NULL;

		if (node->priv == NULL)
		{
			result->set.ref.parent_level = -1;
		}
		else
		{
			result->set.ref = *(scope_ref_t)node->priv;
			free(node->priv);
		}

		break;
	}

	case AST_CALLCC:
	{
		ast_callcc_t s = (ast_callcc_t)(node + 1);
		result->type = EXP_TYPE_CALLCC;
		result->callcc.tail = s->tail;
		result->callcc.exp = expression_from_ast_internal(heap, s->node, handle, priv);
		result->callcc.exp->parent = result;
		result->callcc.exp->next = NULL;
		
		break;
	}

	default:
	{
		break;
	}
		
	}

	free(node);
	
	return result;
}

expression_t
expression_from_ast(heap_t heap, ast_node_t node)
{
	unsigned int exps_size = 0;
	unsigned int objs_size = 0;

	scan_ast(node, &exps_size, &objs_size);
	
	expression_t exps = (expression_t)malloc(sizeof(struct expression_s) * exps_size);
	object_t *objs = (object_t *)malloc(sizeof(object_t) * objs_size);
	
	object_t handle = heap_object_new(heap);
	exp_handle_priv_t priv = (exp_handle_priv_t)malloc(sizeof(struct exp_handle_priv_s));

	priv->exps = exps;
	priv->objs = objs;
	priv->exps_count = 0;
	priv->objs_count = 0;
	
	handle->external.priv = priv;
	handle->external.free = handle_free;
	handle->external.enumerate = handle_enumerate;
	
	expression_t result = expression_from_ast_internal(heap, node, handle, priv);
	result->parent = NULL;
	result->next = NULL;
	
	handle->gc.type = OBJECT_EXTERNAL;

	int i;
	for (i = 0; i < priv->objs_count; ++ i)
	{
		if (priv->objs[i] &&
			is_object(priv->objs[i]))
		{
			heap_unprotect(heap, priv->objs[i]);
		}
	}

	return result;
}
