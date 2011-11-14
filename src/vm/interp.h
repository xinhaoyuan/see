#ifndef __SEE_INTERP_H__
#define __SEE_INTERP_H__

/* All header files that a interp may need */
#include "../object.h"
#include "../as/simple_parse.h"
#include "../as/syntax_parse.h"
#include "../as/free.h"
#include "../as/dump.h"
#include "symref.h"
#include "io.h"
#include "vm.h"

typedef struct interp_s *interp_t;
typedef struct interp_s
{
	heap_t      heap;
	
	execution_t ex;
	object_t    prog;
	object_t    ex_func;
	int         ex_args_size;
	object_t   *ex_args;
	object_t    ret;
} interp_s;

int         interp_initialize(interp_t i, int ex_args_size);
void        interp_uninitialize(interp_t i);
object_t    interp_eval(interp_t i, stream_in_f f, void *data);
execution_t interp_switch(interp_t i, execution_t ex);
object_t    interp_detach(interp_t i);
int         interp_apply(interp_t i, object_t prog, int argc, object_t *args);
int         interp_run(interp_t i, object_t ex_ret, int *ex_argc, object_t **ex_args);
void        interp_protect(interp_t i, object_t object);
void        interp_unprotect(interp_t i, object_t object);

#endif
