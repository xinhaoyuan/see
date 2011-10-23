#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "io.h"
#include "../as/dump.h"

void
object_dump(object_t o)
{
	switch (OBJECT_TYPE(o))
	{
	case ENCODE_SUFFIX_SYMBOL:
		if (o ==  OBJECT_NULL)
			printf(" (NULL)");
		else printf(" (SYMBOL:%p\n)", o);
		break;

	case ENCODE_SUFFIX_INT:
		printf(" %d", INT_UNBOX(o));
		break;

	case ENCODE_SUFFIX_BOXED:
		printf(" (BOXED:%p)", EXTERNAL_UNBOX(o));
		break;
		
	case OBJECT_TYPE_STRING:
		printf(" \"%s\"", xstring_cstr(o->string));
		break;
		
	case OBJECT_TYPE_PAIR:
		printf(" (");
		object_dump(SLOT_GET(o->pair.slot_car));
		printf(" .");
		object_dump(SLOT_GET(o->pair.slot_cdr));
		printf(" )");
		break;
		
	case OBJECT_TYPE_VECTOR:
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
	
	case OBJECT_TYPE_ENVIRONMENT:
		printf(" (ENVIRONMENT)");
		break;
		
	case OBJECT_TYPE_CLOSURE:
		printf(" (CLOSURE)");
		break;
		
	case OBJECT_TYPE_CONTINUATION:
		printf(" (CONTINUATION)");
		break;
		
	case OBJECT_TYPE_EXECUTION:
		printf(" (EXECUTION)");
		break;
		
	case OBJECT_TYPE_EXTERNAL:
		printf(" (EXTERNAL)");
		break;
		
	default:
		printf(" (UNKNOWN)");
		break;
	}
}

static void
scan_ast(ast_node_t node, unsigned int *exps_count, unsigned *objs_count)
{
	++ (*exps_count);
	
	switch (node->header.type)
	{
	case AST_SYMBOL:
	{
		if (node->symbol.type == SYMBOL_STRING)
			++ (*objs_count);
		
		break;
	}

	case AST_LAMBDA:
	{
		scan_ast(node->lambda.proc, exps_count, objs_count);
		break;
	}

	case AST_WITH:
	{
		scan_ast(node->with.proc, exps_count, objs_count);
		break;
	}


	case AST_PROC:
	{
		ast_node_t cur = node->proc.head;
		while (cur != NULL)
		{
			scan_ast(cur, exps_count, objs_count);
			cur = cur->header.next;
			if (cur == node->proc.head) cur = NULL;
		}
		break;
	}

	case AST_AND:
	{
		ast_node_t cur = node->s_and.head;
		while (cur != NULL)
		{
			scan_ast(cur, exps_count, objs_count);
			cur = cur->header.next;
			if (cur == node->proc.head) cur = NULL;
		}
		break;
	}

	case AST_OR:
	{
		ast_node_t cur = node->s_or.head;
		while (cur != NULL)
		{
			scan_ast(cur, exps_count, objs_count);
			cur = cur->header.next;
			if (cur == node->proc.head) cur = NULL;
		}
		break;
	}


	case AST_APPLY:
	{
		int i;
		
		scan_ast(node->apply.func, exps_count, objs_count);
		for (i = 0; i < node->apply.argc; ++ i)
			scan_ast(node->apply.args[i], exps_count, objs_count);
		
		break;
	}

	case AST_COND:
	{
		scan_ast(node->cond.c, exps_count, objs_count);
		scan_ast(node->cond.t, exps_count, objs_count);
		if (node->cond.e != NULL)
			scan_ast(node->cond.e, exps_count, objs_count);

		break;
	}
		
	case AST_SET:
	{
		scan_ast(node->set.value, exps_count, objs_count);
		break;
	}

	case AST_CALLCC:
	{
		scan_ast(node->callcc.node, exps_count, objs_count);
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

/* simple routine to scan a integer in a string */
static int
scan_int(char *string, int *result)
{
	int sign;
	int temp;
	
	switch (*string)
	{
	case 0:
		*result = 0;
		return -1;

	case '-':
		sign = -1;
		++ string;
		break;

	default:
		sign = 1;
		break;
	}

	temp = 0;
	while (*string >= '0' && *string <= '9')
	{
		temp = temp * 10 + (*string - '0');
		++ string;
	}

	*result = temp * sign;
	return 0;
}

static expression_t 
expression_from_ast_internal(heap_t heap, ast_node_t node, object_t handle, exp_handle_priv_t priv)
{
	expression_t result = priv->exps + (priv->exps_count ++);
	result->handle = handle;
	
	switch (node->header.type)
	{
	case AST_SYMBOL:
	{
		int type = node->symbol.type;
		switch (type)
		{
		case SYMBOL_NULL:
			result->type = EXP_TYPE_VALUE;
			result->depth = 1;
			result->value = NULL;
			break;
				
		case SYMBOL_GENERAL:
			result->type = EXP_TYPE_REF;
			result->depth = 1;
			if (node->header.priv == NULL)
			{
				result->ref.parent_level = -1;
			}
			else
			{
				result->ref  = *(scope_ref_t)node->header.priv;
			}
			
			break;

		case SYMBOL_NUMERIC:
		{
			int v;
			
			if (xstring_cstr(node->symbol.str)[0] == '#')
			{
				if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_CONS, -1))
				{
					v = FUNC_CONS;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_CAR, -1))
				{
					v = FUNC_CAR;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_CDR, -1))
				{
					v = FUNC_CDR;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_ADD, -1))
				{
					v = FUNC_ADD;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_SUB, -1))
				{
					v = FUNC_SUB;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_EQ, -1))
				{
					v = FUNC_EQ;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_VEC, -1))
				{
					v = FUNC_VEC;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_VEC_LEN, -1))
				{
					v = FUNC_VEC_LEN;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_VEC_REF, -1))
				{
					v = FUNC_VEC_REF;
				}
				else if (xstring_equal_cstr(node->symbol.str, SYMBOL_CONSTANT_VEC_SET, -1))
				{
					v = FUNC_VEC_SET;
				}
				else
				{
					v = 0;
				}
			}
			else scan_int(xstring_cstr(node->symbol.str), &v);

			result->type = EXP_TYPE_VALUE;
			result->depth = 1;
			result->value = INT_BOX(v);
			break;
		}

		case SYMBOL_STRING:
		{
			result->type = EXP_TYPE_VALUE;
			result->depth = 1;
			result->value = heap_object_new(heap);
			result->value->string = xstring_from_cstr(
				xstring_cstr(node->symbol.str), xstring_len(node->symbol.str));
			OBJECT_TYPE_INIT(result->value, OBJECT_TYPE_STRING);
			
			priv->objs[priv->objs_count ++] = result->value;
			break;
		}

		default:
			printf("unknown type to translate symbol: %d\n", type);
			break;
			
		}

		break;
	}

	case AST_LAMBDA:
	{
		result->type = EXP_TYPE_CLOSURE;
		result->closure.argc = node->lambda.tail_list ? node->lambda.argc : node->lambda.argc + 1;
		/* XXX */
		result->closure.inherit = 1;
		
		result->closure.child = expression_from_ast_internal(heap, node->lambda.proc, handle, priv);
		result->closure.child->parent = result;
		result->closure.child->next = NULL;
		result->depth = 1;
		
		break;
	}

	case AST_WITH:
	{
		result->type = EXP_TYPE_WITH;
		result->with.varc = node->with.varc;
		result->with.inherit = 1;
		
		result->with.child = expression_from_ast_internal(heap, node->with.proc, handle, priv);
		result->with.child->parent = result;
		result->with.child->next = NULL;

		result->depth = result->with.child->depth;

		break;
	}


	case AST_PROC:
	{
		ast_node_t cur = node->proc.head;
		
		result->type = EXP_TYPE_PROC;
		expression_t e = result->proc.child =
			expression_from_ast_internal(heap, cur, handle, priv);
		result->depth = e->depth;

		cur = cur->header.next;
		while (cur != node->proc.head)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, cur, handle, priv);
			e = e->next;

			if (result->depth < e->depth)
				result->depth = e->depth;

			cur = cur->header.next;
		}
		e->parent = result;
		e->next = NULL;

		break;
	}

	case AST_AND:
	{
		ast_node_t cur = node->s_and.head;
		
		result->type = EXP_TYPE_AND;
		expression_t e = result->and_exp.child =
			expression_from_ast_internal(heap, cur, handle, priv);
		result->depth = e->depth;
		
		cur = cur->header.next;
		while (cur != node->s_and.head)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, cur, handle, priv);
			e = e->next;

			if (result->depth < e->depth)
				result->depth = e->depth;

			cur = cur->header.next;
		}
		e->parent = result;
		e->next = NULL;

		break;
	}

	case AST_OR:
	{
		ast_node_t cur = node->s_or.head;
		
		result->type = EXP_TYPE_OR;
		expression_t e = result->or_exp.child =
			expression_from_ast_internal(heap, cur, handle, priv);
		result->depth = e->depth;

		cur = cur->header.next;
		while (cur != node->s_or.head)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, cur, handle, priv);
			e = e->next;

			if (result->depth < e->depth)
				result->depth = e->depth;

			cur = cur->header.next;
		}
		e->parent = result;
		e->next = NULL;

		break;
	}


	case AST_APPLY:
	{
		int i;
		
		result->type = EXP_TYPE_APPLY;
		result->apply.argc = node->apply.argc;
		result->apply.tail = node->apply.tail;
		expression_t e = result->apply.child = expression_from_ast_internal(heap, node->apply.func, handle, priv);
		result->depth = e->depth;
		for (i = 0; i < node->apply.argc; ++ i)
		{
			e->parent = result;
			e->next = expression_from_ast_internal(heap, node->apply.args[i], handle, priv);
			e = e->next;

			if (result->depth < e->depth + i + 1)
				result->depth = e->depth + i + 1;
		}
		e->parent = result;
		e->next = NULL;
		
		break;
	}

	case AST_COND:
	{
		result->type = EXP_TYPE_COND;
		
		result->cond.cd = expression_from_ast_internal(heap, node->cond.c, handle, priv);
		result->cond.cd->parent = result;
		result->cond.cd->next = NULL;
		
		result->cond.th = expression_from_ast_internal(heap, node->cond.t, handle, priv);
		result->cond.th->parent = result;
		result->cond.th->next = NULL;

		result->depth = result->cond.cd->depth > result->cond.th->depth ?
			result->cond.cd->depth : result->cond.th->depth;

		if (node->cond.e != NULL)
		{
			result->cond.el = expression_from_ast_internal(heap, node->cond.e, handle, priv);
			result->cond.el->parent = result;
			result->cond.el->next = NULL;

			if (result->depth < result->cond.el->depth)
				result->depth = result->cond.el->depth;
		}
		else result->cond.el = NULL;

		break;
	}
		
	case AST_SET:
	{
		result->type = EXP_TYPE_SET;
		result->set.exp = expression_from_ast_internal(heap, node->set.value, handle, priv);
		result->set.exp->parent = result;
		result->set.exp->next = NULL;

		result->depth = result->set.exp->depth;

		if (node->header.priv == NULL)
		{
			result->set.ref.parent_level = -1;
		}
		else
		{
			result->set.ref = *(scope_ref_t)node->header.priv;
		}

		break;
	}

	case AST_CALLCC:
	{
		result->type = EXP_TYPE_CALLCC;
		result->callcc.tail = node->callcc.tail;
		result->callcc.exp = expression_from_ast_internal(heap, node->callcc.node, handle, priv);
		result->callcc.exp->parent = result;
		result->callcc.exp->next = NULL;
		result->depth = result->callcc.exp->depth + 1;
		
		break;
	}

	default:
	{
		printf("unknown type to translate ast: %d\n", node->header.type);
		ast_dump(node, stderr);
		break;
	}
		
	}

	return result;
}

object_t
handle_from_ast(heap_t heap, ast_node_t node)
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
	
	OBJECT_TYPE_INIT(handle, OBJECT_TYPE_EXTERNAL);

	int i;
	for (i = 0; i < priv->objs_count; ++ i)
	{
		if (priv->objs[i] &&
			IS_OBJECT(priv->objs[i]))
		{
			heap_unprotect(heap, priv->objs[i]);
		}
	}

	return handle;
}

expression_t
handle_expression_get(object_t handle)
{
	return ((exp_handle_priv_t)(handle->external.priv))->exps;
}
