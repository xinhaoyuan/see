#ifndef __SEE_BUILTIN_H__
#define __SEE_BUILTIN_H__

#define BUILTIN_NOT     1
#define BUILTIN_CAR     2
#define BUILTIN_CDR     3
#define BUILTIN_CONS    4
#define BUILTIN_ADD     5
#define BUILTIN_SUB     6
#define BUILTIN_EQ      7
#define BUILTIN_VEC     8
#define BUILTIN_VEC_LEN 9
#define BUILTIN_VEC_REF 10
#define BUILTIN_VEC_SET 11

#define BUILTIN_MAX     11


typedef object_t(*see_internal_builtin_f)(heap_t heap, int argc, object_t *args, int *error);
extern see_internal_builtin_f see_internal_builtin[BUILTIN_MAX + 1];

#endif
