#include <config.h>

#include "../object.h"

#include "vm.h"
#include "func.h"

#define FUNC(name) static object_t name (heap_t heap, int argc, object_t *args, int *error)

FUNC(func_not)
{
    if (argc == 2)
    {
        if (args[1] == OBJECT_NULL || args[1] == OBJECT_FALSE)
            return OBJECT_TRUE;
        else return OBJECT_FALSE;
    }
    else return OBJECT_NULL;
}

FUNC(func_car)
{
    if (argc == 2 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_PAIR)
            return SLOT_GET(args[1]->pair.slot_car);
        else return OBJECT_NULL;
}

FUNC(func_cdr)
{
    if (argc == 2 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_PAIR)
            return SLOT_GET(args[1]->pair.slot_cdr);
        else return OBJECT_NULL;
}

FUNC(func_cons)
{
    if (argc == 3)
    {
        object_t r = heap_object_new(heap);
        if (r == NULL)
        {
            *error = VM_ERROR_HEAP;
            return NULL;
        }
                
        SLOT_SET(r->pair.slot_car, args[1]);
        SLOT_SET(r->pair.slot_cdr, args[2]);
        OBJECT_TYPE_INIT(r, OBJECT_TYPE_PAIR);
                
        heap_unprotect(heap, r);
        return r;
    }
    else return OBJECT_NULL;
}

FUNC(func_add)
{
    int i, r = 0;
    for (i = 1; i != argc; ++ i)
    {
        if (IS_INT(args[i]))
            r += INT_UNBOX(args[i]);
    }
    
    return INT_BOX(r);
}

FUNC(func_sub)
{
    if (argc == 3)
    {
        return INT_BOX(INT_UNBOX(args[1]) - INT_UNBOX(args[2]));
    }
    else return OBJECT_NULL;
}

FUNC(func_eq)
{
    if (argc == 3)
    {
        return (args[1] == args[2]) ? OBJECT_TRUE : OBJECT_FALSE;
    }
    else return OBJECT_NULL;
}

FUNC(func_vec)
{
    object_t v = heap_object_new(heap);
    if (v == NULL)
    {
        *error = VM_ERROR_HEAP;
        return NULL;
    }
    
    slot_s *entry = (slot_s *)SEE_MALLOC(sizeof(slot_s) * (argc - 1));
    if (entry == NULL)
    {
        heap_object_free(heap, v);
        *error = VM_ERROR_MEMORY;
        return NULL;
    }

    v->vector.length = argc - 1;
    v->vector.slot_entry = entry;

    int i;
    for (i = 1; i < argc; ++ i)
    {
        SLOT_INIT(v->vector.slot_entry[i - 1], args[i]);
    }
    OBJECT_TYPE_INIT(v, OBJECT_TYPE_VECTOR);
    heap_unprotect(heap, v);
    return v;
}

FUNC(func_vec_len)
{
    if (argc == 2 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_VECTOR)
    {
        return INT_BOX(args[1]->vector.length);
    }
    else return INT_BOX(0);
}

FUNC(func_vec_ref)
{
    if (argc == 3 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_VECTOR &&
        IS_INT(args[2]) && INT_UNBOX(args[2]) < args[1]->vector.length)
    {
        return SLOT_GET(args[1]->vector.slot_entry[INT_UNBOX(args[2])]);
    }
    else return OBJECT_NULL;
}

FUNC(func_vec_set)
{
    if (argc == 4 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_VECTOR &&
            IS_INT(args[2]) && INT_UNBOX(args[2]) < args[1]->vector.length)
    {
        SLOT_SET(args[1]->vector.slot_entry[INT_UNBOX(args[2])], args[3]);
    }

    return OBJECT_NULL;
}

see_internal_func_f see_internal_func[FUNC_MAX + 1] =
{
    [FUNC_NOT]     = func_not,
    [FUNC_CAR]     = func_car,
    [FUNC_CDR]     = func_cdr,
    [FUNC_CONS]    = func_cons,
    [FUNC_ADD]     = func_add,
    [FUNC_SUB]     = func_sub,
    [FUNC_EQ]      = func_eq,
    [FUNC_VEC]     = func_vec,
    [FUNC_VEC_LEN] = func_vec_len,
    [FUNC_VEC_REF] = func_vec_ref,
    [FUNC_VEC_SET] = func_vec_set,
};
