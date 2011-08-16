#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <object.h>

#include "../as/symbol.h"
#include "vm.h"

/* The internal function numbers */

static inline unsigned int 
bsr(unsigned int n)
{
	 unsigned int result;
	 asm volatile("bsr %1, %0"
				  : "=r" (result)
				  : "r" (n));
	 return result;
}

#define TRY_PUSH(x)														\
	{																	\
		++ ex->stack_count;												\
		if (ex->stack_alloc < ex->stack_count)							\
		{																\
			ex->stack_alloc <<= 1;										\
			ex->stack = (object_t *)realloc(ex->stack, sizeof(object_t) * ex->stack_alloc); \
		}																\
		ex->stack[ex->stack_count - 1] = (x);							\
	}

static int
apply_internal(heap_t heap, unsigned int argc, execution_t ex, object_t *ex_func, int *ex_argc, object_t **ex_args)
{
	object_t  func = ex->stack[ex->stack_count - argc - 1];
	object_t *args = &ex->stack[ex->stack_count - argc];

	unsigned int type = OBJECT_TYPE(func);
	switch (type)
	{
	case OBJECT_TYPE_CLOSURE:
	{
		object_t tail_vector = OBJECT_NULL;
		object_t new_env = heap_object_new(heap);
		
		new_env->environment.length = func->closure.argc;
		new_env->environment.parent = func->closure.env;
			
		if (new_env->environment.length > 0)
		{
			/* Need to fill the new envonment */
			new_env->environment.slot_entry = (slot_s *)malloc(sizeof(slot_s) * new_env->environment.length);
			if (new_env->environment.length > argc)
			{
				/* Case 1: args not enough */
				int i;
				for (i = 0; i < new_env->environment.length; ++ i)
				{
					if (i < argc)
						SLOT_INIT(new_env->environment.slot_entry[i], args[i]);
					else SLOT_INIT(new_env->environment.slot_entry[i], OBJECT_NULL);
				}
			}
			else
			{
				/* Case 2: args may exceed, and would be passed in the tail vector */
				int i;
				for (i = 0; i < new_env->environment.length - 1; ++ i)
				{
					SLOT_INIT(new_env->environment.slot_entry[i], args[i]);
				}

				tail_vector = heap_object_new(heap);
				tail_vector->vector.length     = argc - new_env->environment.length + 1;
				tail_vector->vector.slot_entry = (slot_s *)malloc(sizeof(slot_s) * tail_vector->vector.length);

				for (i = new_env->environment.length - 1; i < argc; ++ i)
				{
					SLOT_INIT(tail_vector->vector.slot_entry[i - new_env->environment.length + 1], args[i]);
				}

				SLOT_INIT(new_env->environment.slot_entry[new_env->environment.length - 1], tail_vector);
				OBJECT_TYPE_INIT(tail_vector, OBJECT_TYPE_VECTOR);
			}
		}
		else
		{
			new_env->environment.slot_entry = NULL;
		}

		OBJECT_TYPE_INIT(new_env, OBJECT_TYPE_ENVIRONMENT);
		ex->stack_count -= argc + 1;

		if (!(ex->exp->parent->type == EXP_TYPE_APPLY ?
			  ex->exp->parent->apply.tail :
			  ex->exp->parent->callcc.tail))
		{
			TRY_PUSH(EXTERNAL_BOX(ex->exp->parent));
			TRY_PUSH(ex->env);
		}

		ex->exp = func->closure.exp;
		ex->env = new_env;
			
		heap_unprotect(heap, new_env);

		if (tail_vector != OBJECT_NULL) heap_unprotect(heap, tail_vector);
		ex->to_push = 0;
	}
	break;
		
	case OBJECT_TYPE_CONTINUATION:
	{
		if (argc == 0)
		{
			ex->value = OBJECT_NULL;
			ex->to_push = 0;
		}
		else if (argc >= 1)
		{
			ex->value = args[argc - 1];
			ex->to_push = 1;
		}
							
		ex->exp   = func->continuation.exp;
		ex->env = func->continuation.env;

		ex->stack_count = func->continuation.stack_count;
			
		if (ex->stack_count <= 0)
			ex->stack_alloc = 1;
		else ex->stack_alloc  = 1 << (bsr(ex->stack_count) + 1);
			
		ex->stack = (object_t *)realloc(ex->stack, sizeof(object_t) * ex->stack_alloc);
		memcpy(ex->stack, func->continuation.stack, sizeof(object_t) * ex->stack_count);
	}
	break;
		
	case OBJECT_TYPE_STRING:
	{
		/* External calls */
		
		*ex_func = func;
		heap_protect_from_gc(heap, func);
							
		*ex_argc = argc;
		*ex_args = (object_t *)malloc(sizeof(object_t) * argc);
			
		int i;
		for (i = 0; i != argc; ++ i)
		{
			(*ex_args)[i] = args[i];
			if (IS_OBJECT(args[i]))
				heap_protect_from_gc(heap, args[i]);
		}

		ex->exp = ex->exp->parent;
		ex->stack_count -= argc + 1;
		ex->value = OBJECT_NULL;
		ex->to_push = 1;

		return 1;
	}
	break;

	case ENCODE_SUFFIX_INT:
	{
		/* Internal func */
		
		switch (INT_UNBOX(func))
		{
			
		case FUNC_CAR:
		{
			if (argc == 1 && OBJECT_TYPE(args[0]) == OBJECT_TYPE_PAIR)
				ex->value = SLOT_GET(args[0]->pair.slot_car);
			else ex->value = OBJECT_NULL;
							
			break;
		}

		case FUNC_CDR:
		{
			if (argc == 1 && OBJECT_TYPE(args[0]) == OBJECT_TYPE_PAIR)
				ex->value = SLOT_GET(args[0]->pair.slot_cdr);
			else ex->value = OBJECT_NULL;

			break;
		}

		case FUNC_CONS:
		{
			if (argc == 2)
			{
				object_t r = heap_object_new(heap);
				SLOT_SET(r->pair.slot_car, args[0]);
				SLOT_SET(r->pair.slot_cdr, args[1]);
				OBJECT_TYPE_INIT(r, OBJECT_TYPE_PAIR);
				
				ex->value = r;
				heap_unprotect(heap, r);
			}
			else ex->value = OBJECT_NULL;

			break;
		}

		case FUNC_ADD:
		{
			int i, r = 0;
			for (i = 0; i != argc; ++ i)
			{
				if (IS_INT(args[i]))
					r += INT_UNBOX(args[i]);
			}

			ex->value = INT_BOX(r);
			break;
		}

		case FUNC_SUB:
		{
			if (argc == 2)
			{
				ex->value = INT_BOX(INT_UNBOX(args[0]) - INT_UNBOX(args[1]));
			}
			else ex->value = OBJECT_NULL;
			break;
		}

		case FUNC_EQ:
		{
			if (argc == 2)
			{
				ex->value = (args[0] == args[1]) ? INT_BOX(0) : OBJECT_NULL;
			}
			else ex->value = OBJECT_NULL;
			break;
		}

		case FUNC_VEC:
		{
			object_t v = heap_object_new(heap);

			v->vector.length = argc;
			v->vector.slot_entry = (slot_s *)malloc(sizeof(slot_s) * argc);

			int i;
			for (i = 0; i < argc; ++ i)
			{
				SLOT_INIT(v->vector.slot_entry[i], args[i]);
			}
			OBJECT_TYPE_INIT(v, OBJECT_TYPE_VECTOR);
			ex->value = v;

			heap_unprotect(heap, v);
			break;
		}

		case FUNC_VEC_LEN:
		{
			if (argc == 1 && OBJECT_TYPE(args[0]) == OBJECT_TYPE_VECTOR)
			{
				ex->value = INT_BOX(args[0]->vector.length);
			}
			else ex->value = INT_BOX(0);
			break;
		}

		case FUNC_VEC_REF:
		{
			if (argc == 2 && OBJECT_TYPE(args[0]) == OBJECT_TYPE_VECTOR &&
				IS_INT(args[1]) && INT_UNBOX(args[1]) < args[0]->vector.length)
			{
				ex->value = SLOT_GET(args[0]->vector.slot_entry[INT_UNBOX(args[1])]);
			}
			else ex->value = OBJECT_NULL;
			break;
		}

		case FUNC_VEC_SET:
		{
			if (argc == 3 && OBJECT_TYPE(args[0]) == OBJECT_TYPE_VECTOR &&
				IS_INT(args[1]) && INT_UNBOX(args[1]) < args[0]->vector.length)
			{
				SLOT_SET(args[0]->vector.slot_entry[INT_UNBOX(args[1])], args[2]);
			}

			ex->value = OBJECT_NULL;
			break;
		}

		default:
		{
			ex->value = OBJECT_NULL;
			break;
		}

		}

		ex->to_push = 1;
		ex->exp = ex->exp->parent;
		ex->stack_count -= argc + 1;
	}
	break;

	default:
	{
		ex->value = OBJECT_NULL;

		ex->to_push = 1;
		ex->exp = ex->exp->parent;
		ex->stack_count -= argc + 1;

		break;
	}
	}

	return 0;
}

int 
vm_apply(heap_t heap, object_t object, int argc, object_t *args, object_t *ret, execution_t *execution, object_t *ex_func, int *ex_argc, object_t **ex_args)
{
	execution_t ex;
	
	if (*execution == NULL)
	{
		if (!IS_OBJECT(object))
		{
			return APPLY_ERROR;
		}
		else
		{
			ex = *execution = heap_execution_new(heap);
						
			TRY_PUSH(object);
			int i;
			for (i = 0; i != argc; ++ i)
			{
				TRY_PUSH(args[i]);
			}
			
			ex->value = OBJECT_NULL;
			ex->to_push = 0;
			
			OBJECT_TYPE_INIT(ex, OBJECT_TYPE_EXECUTION);
			if (apply_internal(heap, argc, ex, ex_func, ex_argc, ex_args))
				return APPLY_EXTERNAL_CALL;
		}
	}
	else
	{
		ex = *execution;
		
		if (IS_OBJECT(*ex_func))
			heap_unprotect(heap, *ex_func);
		
		int i;
		for (i = 0; i != *ex_argc; ++ i)
		{
			object_t arg = (*ex_args)[i];
			if (IS_OBJECT(arg))
				heap_unprotect(heap, arg);
		}
	}

	while (1)
	{
		if (ex->to_push)
		{
			if (ex->exp == NULL)
			{
				if (IS_OBJECT(ex->value))
					heap_protect_from_gc(heap, ex->value);
				
				heap_execution_free(ex);
				*ret = ex->value;

				*execution = NULL;
				
				return APPLY_EXIT;
			}

			expression_t exp_parent = ex->exp->parent;
			
			if (ex->exp->next != NULL)
			{
				if (exp_parent)
				{
					switch (exp_parent->type)
					{
					case EXP_TYPE_PROC:
						break;

					case EXP_TYPE_AND:
						if (ex->value == OBJECT_NULL)
						{
							ex->exp = exp_parent;
							continue;
						}
						break;

					case EXP_TYPE_OR:
						if (ex->value != OBJECT_NULL)
						{
							ex->exp = exp_parent;
							continue;
						}
						break;
						
					default:
						TRY_PUSH(ex->value);
						break;
					}
				}

				ex->exp = ex->exp->next;
				ex->to_push = 0;
			}
			else if (exp_parent == NULL)
			{
				ex->exp = exp_parent;
			}
			else
			{
				switch (exp_parent->type)
				{
				case EXP_TYPE_AND:
					ex->exp = exp_parent;
					break;

				case EXP_TYPE_OR:
					ex->exp = exp_parent;
					break;

				case EXP_TYPE_SET:
				{

					if (exp_parent->set.ref.parent_level != (unsigned int)-1)
					{
						object_t e = ex->env;
						int i;
						for (i = 0; i != exp_parent->set.ref.parent_level; ++ i)
						{
							if (e == OBJECT_NULL) break;
							e = e->environment.parent;
						}

						if (e != OBJECT_NULL && e->environment.length > exp_parent->set.ref.offset)
							SLOT_SET(e->environment.slot_entry[exp_parent->set.ref.offset], ex->value);
					}
						
					ex->value = OBJECT_NULL;
					ex->exp = exp_parent;
					
					break;
				}

				case EXP_TYPE_COND:
				{
					if (exp_parent->cond.cd == ex->exp)
					{
						if (ex->value != OBJECT_NULL)
						{
							ex->exp = exp_parent->cond.th;
							ex->to_push = 0;
						}
						else if (exp_parent->cond.el != NULL)
						{
							ex->exp = exp_parent->cond.el;
							ex->to_push = 0;
						}
						else
						{
							ex->value = OBJECT_NULL;
							ex->exp = exp_parent;
						}
					}
					else
					{
						ex->exp = exp_parent;
					}
					break;
				}

				case EXP_TYPE_WITH:
				{
					ex->exp = exp_parent;
					break;
				}

				case EXP_TYPE_PROC:
				{
					ex->exp = exp_parent;
					break;
				}

				case EXP_TYPE_CLOSURE:
				{
					ex->exp   = EXTERNAL_UNBOX(ex->stack[ex->stack_count - 2]);
					ex->env = ex->stack[ex->stack_count - 1];

					ex->stack_count -= 2;
					break;
				}

				case EXP_TYPE_CALLCC:
				{
					object_t cont = heap_object_new(heap);
					cont->continuation.exp = exp_parent;
					cont->continuation.env = ex->env;
					cont->continuation.stack_count = ex->stack_count;
					cont->continuation.stack = (object_t *)malloc(sizeof(object_t) * ex->stack_count);
					memcpy(cont->continuation.stack, ex->stack, sizeof(object_t) * ex->stack_count);
					OBJECT_TYPE_INIT(cont, OBJECT_TYPE_CONTINUATION);
					
					TRY_PUSH(ex->value);
					TRY_PUSH(cont);
					heap_unprotect(heap, cont);

					if (apply_internal(heap, 1, ex, ex_func, ex_argc, ex_args))
						return APPLY_EXTERNAL_CALL;
					break;
				}
				
				case EXP_TYPE_APPLY:
				{
					TRY_PUSH(ex->value);
					if (apply_internal(heap, exp_parent->apply.argc, ex, ex_func, ex_argc, ex_args))
						return APPLY_EXTERNAL_CALL;
					break;
				}
				
				}
			}
		}
		else if (ex->exp == NULL)
		{
			heap_detach((object_t)&ex);
			*ret = OBJECT_NULL;
			return APPLY_EXIT_NO_VALUE;
		}
		else
		{
			switch (ex->exp->type)
			{
				
			case EXP_TYPE_VALUE:
			{
				ex->value = ex->exp->value;
				ex->to_push = 1;
				break;
			}

			case EXP_TYPE_REF:
			{
				ex->value = OBJECT_NULL;
				if (ex->exp->ref.parent_level != (unsigned int)-1)
				{
					object_t e = ex->env;
					int i;
					for (i = 0; i != ex->exp->ref.parent_level; ++ i)
					{
						if (e == OBJECT_NULL) break;
						e = e->environment.parent;
					}

					if (e != OBJECT_NULL && e->environment.length > ex->exp->ref.offset)
						ex->value = SLOT_GET(e->environment.slot_entry[ex->exp->ref.offset]);
				}
				ex->to_push = 1;
				
				break;
			}

			case EXP_TYPE_SET:
			{
				ex->exp = ex->exp->set.exp;
				break;
			}

			case EXP_TYPE_COND:
			{
				ex->exp = ex->exp->cond.cd;
				break;
			}

			case EXP_TYPE_WITH:
			{
				object_t new_env = heap_object_new(heap);
				new_env->environment.length = ex->exp->with.varc;
				new_env->environment.slot_entry = (slot_s *)malloc(sizeof(slot_s) * ex->exp->with.varc);
				int i;
				for (i = 0; i != ex->exp->with.varc; ++ i)
					SLOT_INIT(new_env->environment.slot_entry[i], OBJECT_NULL);
				new_env->environment.parent = ex->exp->with.inherit ? ex->env : OBJECT_NULL;

				ex->env = new_env;
				heap_unprotect(heap, new_env);

				ex->exp = ex->exp->with.child;
				break;
			}

			case EXP_TYPE_PROC:
			{
				ex->exp = ex->exp->proc.child;
				break;
			}

			case EXP_TYPE_AND:
			{
				ex->exp = ex->exp->and.child;
				break;
			}

			case EXP_TYPE_OR:
			{
				ex->exp = ex->exp->or.child;
				break;
			}

			case EXP_TYPE_CLOSURE:
			{
				object_t c = heap_object_new(heap);
				c->closure.exp  = ex->exp->closure.child;
				c->closure.argc = ex->exp->closure.argc;
				c->closure.env = ex->exp->closure.inherit ? ex->env : OBJECT_NULL;
				OBJECT_TYPE_INIT(c, OBJECT_TYPE_CLOSURE);

				ex->value = c;
				ex->to_push = 1;
				
				heap_unprotect(heap, c);
				break;
			}

			case EXP_TYPE_APPLY:
			{
				ex->exp = ex->exp->apply.child;
				break;
			}

			case EXP_TYPE_CALLCC:
			{
				ex->exp = ex->exp->callcc.exp;
				break;
			}

			default:
			{
				ex->value = OBJECT_NULL;
				ex->to_push = 1;
				break;
			}
			
			}
		}
	}
}
