#include <config.h>
#include "interp.h"

int
interp_initialize(interp_t i, int ex_args_size)
{
    if (i == NULL) return -1;
    
    i->heap = heap_new();
    i->ex   = NULL;
    i->prog = NULL;
    i->ex_args_size = ex_args_size;
    if ((i->ex_args = (object_t *)SEE_MALLOC(sizeof(object_t) * i->ex_args_size))
        == NULL)
    {
        heap_free(i->heap);
        return -1;
    }

    return 0;
}

void
interp_uninitialize(interp_t i)
{
    if (i == NULL) return;
    if (i->ex != NULL) heap_execution_free(i->ex);
    if (i->heap != NULL) heap_free(i->heap);
    if (i->ex_args != NULL) SEE_FREE(i->ex_args);
}

object_t
interp_eval(interp_t i, stream_in_f f, void *data, external_constant_match_f cm)
{
    if (i == NULL) return NULL;
    if (i->heap == NULL) return NULL;
    
    /* Parse input stream into general AST node */
    ast_node_t n = ast_simple_parse_char_stream(f, data);

    if (n == NULL) return NULL;
    
    /* Parse the AST symbol */
    if (ast_syntax_parse(n, 0) != 0)
    {
        ast_free(n);
        return NULL;
    }
    
    sematic_symref_analyse(n);

    /* Generate the object which encapsulate the compact AST
     * format (expression) to execute */
    object_t handle = handle_from_ast(i->heap, n, cm);
    /* release the ast */
    ast_free(n);

    object_t cont = continuation_from_handle(i->heap, handle);
    heap_unprotect(i->heap, handle);

    return cont;
}

object_t
interp_detach(interp_t i)
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

execution_t
interp_switch(interp_t i, execution_t ex)
{
    execution_t tmp = i->ex;
    i->ex = ex;
    return tmp;
}

int
interp_apply(interp_t i, object_t prog, int argc, object_t *args)
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
interp_run(interp_t i, object_t ex_ret, int *ex_argc, object_t **ex_args)
{
    if (i == NULL) return -1;
    if (i->ex == NULL)
    {
        *ex_argc = 0;
        return VM_EXIT;
    }

    *ex_argc = i->ex_args_size;
    *ex_args = i->ex_args;
    
    i->ex->value = ex_ret;
    if (ex_ret != NULL && IS_OBJECT(ex_ret))
        heap_unprotect(i->heap, ex_ret);
    
    int r = VM_LIMIT_EXCEEDED;
    r = vm_run(i->heap, i->ex, ex_argc, i->ex_args, NULL);

    if (r <= 0 && r != VM_LIMIT_EXCEEDED)
    {
        heap_execution_free(i->ex);
        i->ex = NULL;
    }

    return r;
}

object_t
interp_object_new(interp_t i)
{
    if (i && i->heap)
        return heap_object_new(i->heap);
    else return NULL;
}

void
interp_protect(interp_t i, object_t object)
{
    if (i && i->heap && IS_OBJECT(object))
        heap_protect_from_gc(i->heap, object);
}

void
interp_unprotect(interp_t i, object_t object)
{
    if (i && i->heap && IS_OBJECT(object))
        heap_unprotect(i->heap, object);
}
