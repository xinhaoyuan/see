#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
interpreter_init(interpreter_t i, int ex_args_size)
{
	if (i == NULL) return -1;
	
	i->heap = heap_new();
	i->ex   = NULL;
	i->prog = NULL;
	i->ex_args_size = ex_args_size;
	if ((i->ex_args = (object_t *)malloc(sizeof(object_t) * i->ex_args_size))
		== NULL)
	{
		heap_free(i->heap);
		return -1;
	}

	return 0;
}

void
interpreter_clear(interpreter_t i)
{
	if (i == NULL) return;
	if (i->ex != NULL) heap_execution_free(i->ex);
	heap_free(i->heap);
	free(i->ex_args);
}

object_t
interpreter_eval(interpreter_t i, stream_in_f f, void *data)
{
	if (i == NULL) return NULL;
	if (i->heap == NULL) return NULL;
	
	/* Parse input stream into general AST node */
	ast_node_t n = ast_simple_parse_char_stream(f, data);
		
	/* Parse the AST symbol */
	ast_syntax_parse(n, 0);
	sematic_symref_analyse(n);

	/* Generate the object which encapsulate the compact AST
	 * format (expression) to execute */
	object_t handle = handle_from_ast(i->heap, n);
	/* release the ast */
	ast_free(n);

	object_t cont = continuation_from_handle(i->heap, handle);
	heap_unprotect(i->heap, handle);

	return cont;
}

object_t
interpreter_detach(interpreter_t i)
{
	if (i == NULL) return NULL;
	if (i->ex == NULL) return NULL;
	
	object_t cont = continuation_from_execution(i->heap, i->ex);
	if (cont != NULL)
	{
		heap_execution_free(i->ex);
		i->ex = NULL;
	}

	return cont;
}

int
interpreter_apply(interpreter_t i, object_t prog, int argc, object_t *args)
{
	if (i == NULL) return -1;
	if (i->ex != NULL) return -1;
	if ((i->ex = heap_execution_new(i->heap)) == NULL) return -1;
	if (EX_TRY_EXPAND(i->ex, argc + 1)) return -1;

	int t;
	for (t = 0; t < argc + 1; ++ t)
	{
		if (t == 0)
			i->ex->stack[t] = prog;
		else i->ex->stack[t] = args[t - 1];
	}

	i->ex->stack_count = argc + 1;
	
	i->ex->exp = NULL;
	i->ex->value = OBJECT_NULL;
	i->ex->to_push = 0;

	OBJECT_TYPE_INIT(i->ex, OBJECT_TYPE_EXECUTION);

	for (t = 0; t < argc + 1; ++ t)
	{
		if (IS_OBJECT(i->ex->stack[t]))
			heap_unprotect(i->heap, i->ex->stack[t]);
	}
	
	return 0;
}

int
interpreter_run(interpreter_t i, object_t ex_ret, int *ex_argc, object_t **ex_args)
{
	if (i == NULL) return -1;
	if (i->ex == NULL) return -1;

	if (*ex_argc > 0 && *ex_args == i->ex_args)
	{
		int t;
		for (t = 0; t < *ex_argc; ++ t)
		{
			if (IS_OBJECT((*ex_args)[t]))
				heap_unprotect(i->heap, (*ex_args)[t]);
		}
	}

	*ex_argc = i->ex_args_size;
	*ex_args = i->ex_args;
	
	i->ex->value = ex_ret;
	
	int r = vm_run(i->heap, i->ex, ex_argc, i->ex_args);

	if (r != APPLY_EXTERNAL_CALL)
	{
		heap_execution_free(i->ex);
		i->ex = NULL;
	}

	return r;
}
