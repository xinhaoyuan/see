#ifndef __SEE_VM_H__
#define __SEE_VM_H__

#include "../object.h"

#include "../lib/xstring.h"

#define VM_ERROR_STACK      -4
#define VM_ERROR_HEAP       -3
#define VM_ERROR_MEMORY     -2
#define VM_LIMIT_EXCEEDED   -1
#define VM_EXIT              0
#define VM_EXTERNAL_CALL     1
#define VM_EXTERNAL_CONSTANT 2
#define VM_EXTERNAL_LOAD     3
#define VM_EXTERNAL_STORE    4

int vm_run(heap_t heap, execution_t ex, int *ex_argc, object_t *ex_args, int *stop_flag);

#endif
