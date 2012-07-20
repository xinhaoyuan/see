#ifndef __SEE_FUNC_H__
#define __SEE_FUNC_H__

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

#define FUNC_MAX     11


typedef object_t(*see_internal_func_f)(heap_t heap, int argc, object_t *args, int *error);
extern see_internal_func_f see_internal_func[FUNC_MAX + 1];

#endif
