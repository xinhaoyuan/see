#ifndef __SEE_INTERPRETER_H__
#define __SEE_INTERPRETER_H__

/* All header files that a interpreter may need */
#include "../object.h"
#include "../as/simple_parse.h"
#include "../as/syntax_parse.h"
#include "../as/free.h"
#include "../as/dump.h"
#include "symref.h"
#include "io.h"
#include "vm.h"

typedef struct interpreter_s *interpreter_t;
typedef struct interpreter_s
{
	heap_t      heap;
	
	execution_t ex;
	object_t    prog;
	object_t    ex_func;
	int         ex_args_size;
	object_t   *ex_args;
	object_t    ret;
} interpreter_s;

/* Initialize the struct data */
int  interpreter_init(interpreter_t i, int ex_args_size);
void interpreter_clear(interpreter_t i);

object_t interpreter_eval(interpreter_t i, stream_in_f f, void *data);

execution_t interpreter_switch(interpreter_t i, execution_t ex);
/* Detach the current run state of the interpreter into a continuation */
object_t interpreter_detach(interpreter_t i);
/* Attach any object to be apply */
int      interpreter_apply(interpreter_t i, object_t prog, int argc, object_t *args);
/* Push the interpreter to run */
int      interpreter_run(interpreter_t i, object_t ex_ret, int *ex_argc, object_t **ex_args);

void interpreter_protect(interpreter_t i, object_t object);
void interpreter_unprotect(interpreter_t i, object_t object);

#endif
