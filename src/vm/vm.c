#include <config.h>
#include "../object.h"

#include "vm.h"
#include "builtin.h"

/* The internal function numbers */

static inline unsigned int 
bsr(unsigned int n)
{
    unsigned int result;
    asm volatile("bsr %1, %0"
                 : "=r" (result)
                 : "r" (n));
    return result;
}

#define PUSH(x)                                            \
    do {                                                   \
        if (ex->stack_count >= ex->stack_alloc)            \
        {                                                  \
            ERROR("FATAL ERROR IN PUSH, OUT OF RANGE\n");  \
            exit(-1);                                      \
        }                                                  \
        ex->stack[ex->stack_count ++] = (x);               \
    } while (0)

static int
apply_internal(heap_t heap, execution_t ex, unsigned int argc, int *ex_argc, object_t *ex_args)
{
    object_t  func = ex->stack[ex->stack_count - argc - 1];

    unsigned int type = OBJECT_TYPE(func);
    switch (type)
    {
    case OBJECT_TYPE_CLOSURE:
    {
        int eargc = func->closure.argc;
        int depth = func->closure.exp->depth;
        slot_s *tail_slots = NULL;
        slot_s *env_slots  = NULL;
        object_t tail_vector;
        object_t env;

        if ((tail_vector = heap_object_new(heap)) == NULL)
            return VM_ERROR_HEAP;

        if ((env = heap_object_new(heap)) == NULL)
        {
            heap_object_free(heap, tail_vector);
            return VM_ERROR_HEAP;
        }
        
        if (EX_TRY_EXPAND(ex, -argc + 1 + depth))
        {
            /* Cannot expand the stack of ex */
            heap_object_free(heap, tail_vector);
            heap_object_free(heap, env);
            return VM_ERROR_STACK;
        }

        if (eargc > 0)
        {
            env_slots = (slot_s *)SEE_MALLOC(sizeof(slot_s) * eargc);
            if (env_slots == NULL)
            {
                /* MALLOC ERROR */
                heap_object_free(heap, tail_vector);
                heap_object_free(heap, env);
                return VM_ERROR_MEMORY;
            }

            if (eargc <= argc)
            {
                tail_slots = (slot_s *)SEE_MALLOC(sizeof(slot_s) * (argc - eargc + 1));
                if (tail_slots == NULL)
                {
                    /* MALLOC ERROR */
                    SEE_FREE(env_slots);
                    heap_object_free(heap, tail_vector);
                    heap_object_free(heap, env);
                    return VM_ERROR_MEMORY;
                }
            }
        }
                
        env->environment.length = func->closure.argc;
        env->environment.parent = func->closure.env;
            
        if (eargc > 0)
        {
            /* Need to fill the new envonment */
            env->environment.slot_entry = env_slots;
            if (eargc > argc)
            {
                /* Case 1: args not enough */
                int i;
                for (i = 0; i < eargc; ++ i)
                {
                    if (i < argc)
                        SLOT_INIT(env_slots[i], ex->stack[ex->stack_count - argc + i]);
                    else SLOT_INIT(env_slots[i], OBJECT_NULL);
                }

                heap_object_free(heap, tail_vector);
                tail_vector = OBJECT_NULL;
            }
            else
            {
                /* Case 2: args may exceed, and would be passed in the tail vector */
                int i;
                for (i = 0; i < env->environment.length - 1; ++ i)
                {
                    SLOT_INIT(env->environment.slot_entry[i],
                              ex->stack[ex->stack_count - argc + i]);
                }

                tail_vector->vector.length     = argc - eargc + 1;
                tail_vector->vector.slot_entry = tail_slots;

                for (i = eargc - 1; i < argc; ++ i)
                {
                    SLOT_INIT(tail_vector->vector.slot_entry[i - eargc + 1],
                              ex->stack[ex->stack_count - argc + i]);
                }

                SLOT_INIT(env->environment.slot_entry[eargc - 1], tail_vector);
                OBJECT_TYPE_INIT(tail_vector, OBJECT_TYPE_VECTOR);
            }
        }
        else
        {
            env->environment.slot_entry = NULL;
            heap_object_free(heap, tail_vector);
            tail_vector = OBJECT_NULL;
        }

        OBJECT_TYPE_INIT(env, OBJECT_TYPE_ENVIRONMENT);
        ex->stack_count -= argc + 1;

        if (ex->exp == NULL)
        {
            PUSH(EXTERNAL_BOX(NULL));
            PUSH(OBJECT_NULL);
        }
        else if (!(ex->exp->parent->type == EXP_TYPE_APPLY ?
                   ex->exp->parent->apply.tail :
                   ex->exp->parent->callcc.tail))
        {
            PUSH(EXTERNAL_BOX(ex->exp->parent));
            PUSH(ex->env);
        }

        ex->exp = func->closure.exp;
        ex->env = env;
            
        heap_unprotect(heap, env);

        if (tail_vector != OBJECT_NULL) heap_unprotect(heap, tail_vector);
        ex->to_push = 0;
    }
    break;
        
    case OBJECT_TYPE_CONTINUATION:
    {
        /* MAGIC HERE @_@ */
        unsigned int stack_alloc =
            (unsigned int)((see_uint_t)func->continuation.stack[func->continuation.stack_count - 1]);
        object_t *stack = (object_t *)SEE_MALLOC(
            sizeof(object_t) * stack_alloc);

        if (stack == NULL)
            return VM_ERROR_MEMORY;
                                                     
        if (argc == 0)
        {
            ex->value = OBJECT_NULL;
            ex->to_push = 0;
        }
        else if (argc >= 1)
        {
            ex->value = ex->stack[ex->stack_count - 1];
            ex->to_push = 1;
        }
                            
        ex->exp = func->continuation.exp;
        ex->env = func->continuation.env;

        ex->stack_count = func->continuation.stack_count - 1;
        ex->stack_alloc = stack_alloc;
        if (ex->stack) SEE_FREE(ex->stack);
        ex->stack = stack;
        SEE_MEMCPY(ex->stack, func->continuation.stack, sizeof(object_t) * ex->stack_count);
    }
    break;

    case ENCODE_SUFFIX_SYMBOL:
    case OBJECT_TYPE_STRING:
    {
        if (*ex_argc < 1) return VM_ERROR_MEMORY;
        /* External calls */
        ex_args[0] = func;
        if (IS_OBJECT(func))
            heap_protect_from_gc(heap, func);
                            
        int i;
        for (i = 0; i != argc; ++ i)
        {
            object_t arg = ex->stack[ex->stack_count - argc + i];
            if (i + 1 < *ex_argc)
                ex_args[i + 1] = arg;

            if (IS_OBJECT(arg))
                heap_protect_from_gc(heap, arg);
        }

        if (*ex_argc > argc) *ex_argc = argc + 1;

        ex->exp = ex->exp->parent;
        ex->stack_count -= argc + 1;
        ex->value = OBJECT_NULL;
        ex->to_push = 1;

        return 1;
    }
    break;

    case ENCODE_SUFFIX_INT:
    {
        /* Builtin functions */
        int builtin_id = INT_UNBOX(func);
        object_t *args = ex->stack + ex->stack_count - argc - 1;
        int err = 0;
        
        if (builtin_id < 0 || builtin_id > BUILTIN_MAX || see_internal_builtin[builtin_id] == NULL)
            ex->value = OBJECT_NULL;
        else ex->value = see_internal_builtin[builtin_id](heap, argc + 1, args, &err);

        if (ex->value == NULL) return err;

        ex->to_push = 1;
        ex->exp = ex->exp->parent;
        ex->stack_count -= argc + 1;
        
        break;
    }

    default:
    {
        ex->value = OBJECT_NULL;

        ex->to_push = 1;
        ex->exp = ex->exp->parent;
        ex->stack_count -= argc + 1;

        break;
    }
    }

    return 0;
}

int 
vm_run(heap_t heap, execution_t ex, int *ex_argc, object_t *ex_args, int *stop_flag)
{
    int nonstop = 0;
    if (stop_flag == NULL) stop_flag = &nonstop;
    while (!(*stop_flag))
    {
        if (ex->to_push)
        {
            if (ex->exp == NULL)
            {
                if (IS_OBJECT(ex->value))
                    heap_protect_from_gc(heap, ex->value);

                *ex_argc = 1;
                ex_args[0] = ex->value;
                return VM_EXIT;
            }

            if (ex->exp->type == EXP_TYPE_CONSTANT_EXTERNAL)
            {
                ex->exp->type = EXP_TYPE_CONSTANT;
                ex->exp->constant.value = ex->value;
            }

            expression_t exp_parent = ex->exp->parent;
            
            if (ex->exp->next != NULL)
            {
                if (exp_parent)
                {
                    switch (exp_parent->type)
                    {
                    case EXP_TYPE_PROC:
                        break;

                    case EXP_TYPE_AND:
                        if (ex->value == OBJECT_NULL||
                            ex->value == OBJECT_FALSE)
                        {
                            ex->exp = exp_parent;
                            continue;
                        }
                        break;

                    case EXP_TYPE_OR:
                        if (ex->value != OBJECT_NULL &&
                            ex->value != OBJECT_FALSE)
                        {
                            ex->exp = exp_parent;
                            continue;
                        }
                        break;
                        
                    default:
                        PUSH(ex->value);
                        break;
                    }
                }

                ex->exp = ex->exp->next;
                ex->to_push = 0;
            }
            else if (exp_parent == NULL)
            {
                ex->exp = exp_parent;
            }
            else
            {
                switch (exp_parent->type)
                {
                case EXP_TYPE_AND:
                    ex->exp = exp_parent;
                    break;

                case EXP_TYPE_OR:
                    ex->exp = exp_parent;
                    break;

                case EXP_TYPE_STORE_EXTERNAL:
                    ex->exp = exp_parent;
                    return VM_EXTERNAL_STORE;

                case EXP_TYPE_STORE:
                {

                    object_t e = ex->env;
                    int i;
                    for (i = 0; i != exp_parent->store.ref.parent_level; ++ i)
                    {
                        if (e == OBJECT_NULL) break;
                        e = e->environment.parent;
                    }
                    
                    if (e != OBJECT_NULL && e->environment.length > exp_parent->store.ref.offset)
                        SLOT_SET(e->environment.slot_entry[exp_parent->store.ref.offset], ex->value);
                
                    ex->value = OBJECT_NULL;
                    ex->exp = exp_parent;
                    
                    break;
                }

                case EXP_TYPE_COND:
                {
                    if (exp_parent->cond.cd == ex->exp)
                    {
                        if (ex->value != OBJECT_NULL &&
                            ex->value != OBJECT_FALSE)
                        {
                            ex->exp = exp_parent->cond.th;
                            ex->to_push = 0;
                        }
                        else if (exp_parent->cond.el != NULL)
                        {
                            ex->exp = exp_parent->cond.el;
                            ex->to_push = 0;
                        }
                        else
                        {
                            ex->value = OBJECT_NULL;
                            ex->exp = exp_parent;
                        }
                    }
                    else
                    {
                        ex->exp = exp_parent;
                    }
                    break;
                }

                case EXP_TYPE_WITH:
                {
                    ex->exp = exp_parent;
                    ex->env = ex->env->environment.parent;
                    break;
                }

                case EXP_TYPE_PROC:
                {
                    if (exp_parent->proc.toplevel)
                    {
                        ex->env = ex->stack[ex->stack_count - 1];
                        --ex->stack_count;
                    }
                    ex->exp = exp_parent;
                    break;
                }

                case EXP_TYPE_CLOSURE:
                {
                    ex->exp = EXTERNAL_UNBOX(ex->stack[ex->stack_count - 2]);
                    ex->env = ex->stack[ex->stack_count - 1];

                    ex->stack_count -= 2;
                    break;
                }

                case EXP_TYPE_CALLCC:
                {
                    object_t cont = continuation_from_execution(heap, ex);
                    
                    PUSH(ex->value);
                    PUSH(cont);
                    heap_unprotect(heap, cont);

                    int r = apply_internal(heap, ex, 1, ex_argc, ex_args);
                    if (r > 0) return VM_EXTERNAL_CALL;
                    else if (r < 0) return r;

                    break;
                }
                
                case EXP_TYPE_APPLY:
                {
                    PUSH(ex->value);
                    int r = apply_internal(heap, ex, exp_parent->apply.argc, ex_argc, ex_args);
                    if (r > 0) return VM_EXTERNAL_CALL;
                    else if (r < 0) return r;
                    
                    break;
                }
                
                }
            }
        }
        else if (ex->exp == NULL)
        {
            int r = apply_internal(heap, ex, ex->stack_count - 1, ex_argc, ex_args);
            if (r > 0) return VM_EXTERNAL_CALL;
            else if (r < 0) return r;
        }
        else
        {
            switch (ex->exp->type)
            {
                
            case EXP_TYPE_CONSTANT:
            {
                ex->value = ex->exp->constant.value;
                ex->to_push = 1;
                break;
            }

            case EXP_TYPE_CONSTANT_EXTERNAL:
            {
                ex->to_push = 1;
                return VM_EXTERNAL_CONSTANT;
            }

            case EXP_TYPE_LOAD_EXTERNAL:
            {
                ex->to_push = 1;
                return VM_EXTERNAL_LOAD;
            }
            
            case EXP_TYPE_LOAD:
            {
                ex->value = OBJECT_NULL;
                object_t e = ex->env;
                int i;
                for (i = 0; i != ex->exp->load.ref.parent_level; ++ i)
                {
                    if (e == OBJECT_NULL) break;
                        e = e->environment.parent;
                }
                
                if (e != OBJECT_NULL && e->environment.length > ex->exp->load.ref.offset)
                    ex->value = SLOT_GET(e->environment.slot_entry[ex->exp->load.ref.offset]);
                ex->to_push = 1;
                
                break;
            }

            case EXP_TYPE_STORE_EXTERNAL:
            case EXP_TYPE_STORE:
            {
                ex->exp = ex->exp->store.exp;
                break;
            }

            case EXP_TYPE_COND:
            {
                ex->exp = ex->exp->cond.cd;
                break;
            }

            case EXP_TYPE_WITH:
            {
                object_t env = heap_object_new(heap);
                if (env == NULL) return VM_ERROR_HEAP;

                slot_s *entry = (slot_s *)SEE_MALLOC(sizeof(slot_s) * ex->exp->with.varc);
                if (entry == NULL)
                {
                    heap_object_free(heap, env);
                    return VM_ERROR_MEMORY;
                }

                env->environment.length = ex->exp->with.varc;
                env->environment.slot_entry = entry;
                int i;
                for (i = 0; i != ex->exp->with.varc; ++ i)
                    SLOT_INIT(env->environment.slot_entry[i], OBJECT_NULL);
                env->environment.parent = ex->exp->with.inherit ? ex->env : OBJECT_NULL;
                OBJECT_TYPE_INIT(env, OBJECT_TYPE_ENVIRONMENT);

                ex->env = env;

                heap_unprotect(heap, env);

                ex->exp = ex->exp->with.child;
                break;
            }

            case EXP_TYPE_PROC:
            {
                if (ex->exp->proc.toplevel)
                {
                    PUSH(ex->env);
                    ex->env = OBJECT_NULL;
                }
                ex->exp = ex->exp->proc.child;
                break;
            }

            case EXP_TYPE_AND:
            {
                ex->exp = ex->exp->and_exp.child;
                break;
            }

            case EXP_TYPE_OR:
            {
                ex->exp = ex->exp->or_exp.child;
                break;
            }

            case EXP_TYPE_CLOSURE:
            {
                object_t c = heap_object_new(heap);
                if (c == NULL) return VM_ERROR_HEAP;
                
                c->closure.exp  = ex->exp->closure.child;
                c->closure.argc = ex->exp->closure.argc;
                c->closure.env = ex->exp->closure.inherit ? ex->env : OBJECT_NULL;
                OBJECT_TYPE_INIT(c, OBJECT_TYPE_CLOSURE);

                ex->value = c;
                ex->to_push = 1;
                
                heap_unprotect(heap, c);
                break;
            }

            case EXP_TYPE_APPLY:
            {
                ex->exp = ex->exp->apply.child;
                break;
            }

            case EXP_TYPE_CALLCC:
            {
                ex->exp = ex->exp->callcc.exp;
                break;
            }

            default:
            {
                ex->value = OBJECT_NULL;
                ex->to_push = 1;
                break;
            }
            
            }
        }
    }

    return VM_LIMIT_EXCEEDED;
}
