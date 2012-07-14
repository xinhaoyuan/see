#ifndef __SEE_VM_H__
#define __SEE_VM_H__

#include "../object.h"

#include "../lib/xstring.h"

#define APPLY_ERROR            -2
#define APPLY_LIMIT_EXCEEDED   -1
#define APPLY_EXIT              0
#define APPLY_EXTERNAL_CALL     1
#define APPLY_EXTERNAL_CONSTANT 2
#define APPLY_EXTERNAL_LOAD     3
#define APPLY_EXTERNAL_STORE    4

int vm_run(heap_t heap, execution_t ex, int *ex_argc, object_t *ex_args, int *stop_flag);

#define FUNC_NOT     1
#define FUNC_CAR     2
#define FUNC_CDR     3
#define FUNC_CONS    4
#define FUNC_ADD     5
#define FUNC_SUB     6
#define FUNC_EQ      7
#define FUNC_VEC     8
#define FUNC_VEC_LEN 9
#define FUNC_VEC_REF 10
#define FUNC_VEC_SET 11

#endif
