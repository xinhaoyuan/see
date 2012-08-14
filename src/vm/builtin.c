#include <config.h>

#include "../object.h"

#include "vm.h"
#include "builtin.h"

#define BUILTIN(name) static object_t name (heap_t heap, int argc, object_t *args, int *error)

BUILTIN(builtin_not)
{
    if (argc == 2)
    {
        if (args[1] == OBJECT_NULL || args[1] == OBJECT_FALSE)
            return OBJECT_TRUE;
        else return OBJECT_FALSE;
    }
    else return OBJECT_NULL;
}

BUILTIN(builtin_car)
{
    if (argc == 2 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_PAIR)
        return SLOT_GET(args[1]->pair.slot_car);
    else return OBJECT_NULL;
}

BUILTIN(builtin_cdr)
{
    if (argc == 2 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_PAIR)
        return SLOT_GET(args[1]->pair.slot_cdr);
    else return OBJECT_NULL;
}

BUILTIN(builtin_cons)
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

BUILTIN(builtin_add)
{
    int i, r = 0;
    for (i = 1; i != argc; ++ i)
    {
        if (IS_INT(args[i]))
            r += INT_UNBOX(args[i]);
    }
    
    return INT_BOX(r);
}

BUILTIN(builtin_sub)
{
    if (argc == 3 && IS_INT(args[1]) && IS_INT(args[2]))
    {
        return INT_BOX(INT_UNBOX(args[1]) - INT_UNBOX(args[2]));
    }
    else return OBJECT_NULL;
}

BUILTIN(builtin_eq)
{
    if (argc == 3)
    {
        return (args[1] == args[2]) ? OBJECT_TRUE : OBJECT_FALSE;
    }
    else return OBJECT_NULL;
}

BUILTIN(builtin_vec)
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

BUILTIN(builtin_vec_len)
{
    if (argc == 2 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_VECTOR)
    {
        return INT_BOX(args[1]->vector.length);
    }
    else return INT_BOX(0);
}

BUILTIN(builtin_vec_ref)
{
    if (argc == 3 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_VECTOR &&
        IS_INT(args[2]) && INT_UNBOX(args[2]) < args[1]->vector.length)
    {
        return SLOT_GET(args[1]->vector.slot_entry[INT_UNBOX(args[2])]);
    }
    else return OBJECT_NULL;
}

BUILTIN(builtin_vec_set)
{
    if (argc == 4 && OBJECT_TYPE(args[1]) == OBJECT_TYPE_VECTOR &&
            IS_INT(args[2]) && INT_UNBOX(args[2]) < args[1]->vector.length)
    {
        SLOT_SET(args[1]->vector.slot_entry[INT_UNBOX(args[2])], args[3]);
    }

    return OBJECT_NULL;
}

see_internal_builtin_f see_internal_builtin[BUILTIN_MAX + 1] =
{
    [BUILTIN_NOT]     = builtin_not,
    [BUILTIN_CAR]     = builtin_car,
    [BUILTIN_CDR]     = builtin_cdr,
    [BUILTIN_CONS]    = builtin_cons,
    [BUILTIN_ADD]     = builtin_add,
    [BUILTIN_SUB]     = builtin_sub,
    [BUILTIN_EQ]      = builtin_eq,
    [BUILTIN_VEC]     = builtin_vec,
    [BUILTIN_VEC_LEN] = builtin_vec_len,
    [BUILTIN_VEC_REF] = builtin_vec_ref,
    [BUILTIN_VEC_SET] = builtin_vec_set,
};
