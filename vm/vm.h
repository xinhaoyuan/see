#ifndef __SEE_VM_H__
#define __SEE_VM_H__

#include "object.h"

#include <xstring.h>

#define APPLY_ERROR        -1
#define APPLY_EXIT          0
#define APPLY_EXIT_NO_VALUE 1
#define APPLY_EXTERNAL_CALL 2

int vm_apply(heap_t heap, object_t object, int argc, object_t *args, object_t *ret, execution_t ex_external, object_t *ex_func, int *ex_argc, object_t **ex_args);

#endif
