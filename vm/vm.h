#ifndef __SEE_VM_H__
#define __SEE_VM_H__

#include "../object.h"

#include "../lib/xstring.h"

#define APPLY_ERROR        -1
#define APPLY_EXIT          0
#define APPLY_EXIT_NO_VALUE 1
#define APPLY_EXTERNAL_CALL 2

int vm_apply(heap_t heap, object_t object, int argc, object_t *args, object_t *ret, execution_t *execution, object_t *ex_func, int *ex_argc, object_t **ex_args);

#define FUNC_CAR     1
#define FUNC_CDR     2
#define FUNC_CONS    3
#define FUNC_ADD     4
#define FUNC_SUB     5
#define FUNC_EQ      6
#define FUNC_VEC     7
#define FUNC_VEC_LEN 8
#define FUNC_VEC_REF 9
#define FUNC_VEC_SET 10

#define SYMBOL_CONSTANT_CAR     "#car"
#define SYMBOL_CONSTANT_CDR     "#cdr"
#define SYMBOL_CONSTANT_CONS    "#cons"
#define SYMBOL_CONSTANT_ADD     "#add"
#define SYMBOL_CONSTANT_SUB     "#sub"
#define SYMBOL_CONSTANT_EQ      "#eq"
#define SYMBOL_CONSTANT_VEC     "#vec"
#define SYMBOL_CONSTANT_VEC_LEN "#veclen"
#define SYMBOL_CONSTANT_VEC_REF "#vecref"
#define SYMBOL_CONSTANT_VEC_SET "#vecset"

#endif
