#include <config.h>
#include "vm.h"
#include "func.h"
#include "io.h"
#include "../as/dump.h"

struct constant_item_s
{
    const char *name;
    int type;
    see_int_t value;
};

static struct constant_item_s constants[] =
{
    { .name = "#nil",    .type = ENCODE_SUFFIX_SYMBOL, .value = OBJECT_ID_NULL },
    { .name = "#t",      .type = ENCODE_SUFFIX_SYMBOL, .value = OBJECT_ID_TRUE },
    { .name = "#f",      .type = ENCODE_SUFFIX_SYMBOL, .value = OBJECT_ID_FALSE },
    
    { .name = "#not",    .type = ENCODE_SUFFIX_INT, .value = FUNC_NOT },
    { .name = "#not",    .type = ENCODE_SUFFIX_INT, .value = FUNC_NOT },
    { .name = "#car",    .type = ENCODE_SUFFIX_INT, .value = FUNC_CAR },
    { .name = "#cdr",    .type = ENCODE_SUFFIX_INT, .value = FUNC_CDR },
    { .name = "#cons",   .type = ENCODE_SUFFIX_INT, .value = FUNC_CONS },
    { .name = "#add",    .type = ENCODE_SUFFIX_INT, .value = FUNC_ADD },
    { .name = "#sub",    .type = ENCODE_SUFFIX_INT, .value = FUNC_SUB },
    { .name = "#eq",     .type = ENCODE_SUFFIX_INT, .value = FUNC_EQ },
    { .name = "#vec",    .type = ENCODE_SUFFIX_INT, .value = FUNC_VEC },
    { .name = "#veclen", .type = ENCODE_SUFFIX_INT, .value = FUNC_VEC_LEN },
    { .name = "#vecref", .type = ENCODE_SUFFIX_INT, .value = FUNC_VEC_REF },
    { .name = "#vecset", .type = ENCODE_SUFFIX_INT, .value = FUNC_VEC_SET },
};

static object_t
internal_constant_match(xstring_t name)
{
    int i;
    for (i = 0; i < sizeof(constants) / sizeof(struct constant_item_s); ++ i)
    {
        if (xstring_equal_cstr(name, constants[i].name, -1))
        {
            switch (constants[i].type)
            {
            case ENCODE_SUFFIX_INT:
                return INT_BOX(constants[i].value);

            case ENCODE_SUFFIX_SYMBOL:
                return SYMBOL_BOX(constants[i].value);
            }
        }
    }
    return NULL;
}


#if SEE_SYSTEM_IO
void
object_dump(object_t o, void *stream)
{
    switch (OBJECT_TYPE(o))
    {
    case ENCODE_SUFFIX_SYMBOL:
    {
        see_uint_t id = SYMBOL_UNBOX(o);
        switch (id)
        {
        case OBJECT_ID_NULL:
            SEE_FPRINTF(stream, " #nil");
            break;

        case OBJECT_ID_FALSE:
            SEE_FPRINTF(stream, " #f");
            break;

        case OBJECT_ID_TRUE:
            SEE_FPRINTF(stream, " #t");
            break;

        default:
            SEE_FPRINTF(stream, " (SYMBOL:%p)", (void *)id);
            break;
        }

        break;
    }

    case ENCODE_SUFFIX_INT:
#if defined(__i386__)
        SEE_FPRINTF(stream, " %ld", INT_UNBOX(o));
#elif defined(__x86_64__)
        SEE_FPRINTF(stream, " %lld", INT_UNBOX(o));
#endif
        break;

    case ENCODE_SUFFIX_BOXED:
        SEE_FPRINTF(stream, " (BOXED:%p)", EXTERNAL_UNBOX(o));
        break;
        
    case OBJECT_TYPE_STRING:
        SEE_FPRINTF(stream, " \"%s\"", xstring_cstr(o->string));
        break;
        
    case OBJECT_TYPE_PAIR:
        SEE_FPRINTF(stream, " (");
        object_dump(SLOT_GET(o->pair.slot_car), stream);
        SEE_FPRINTF(stream, " .");
        object_dump(SLOT_GET(o->pair.slot_cdr), stream);
        SEE_FPRINTF(stream, " )");
        break;
        
    case OBJECT_TYPE_VECTOR:
    {
        SEE_FPRINTF(stream, " [");
        int i;
        for (i = 0; i != o->vector.length; ++ i)
        {
            object_dump(SLOT_GET(o->vector.slot_entry[i]), stream);
        }
        SEE_FPRINTF(stream, " ]");
        break;
    }
    
    case OBJECT_TYPE_ENVIRONMENT:
        SEE_FPRINTF(stream, " (ENVIRONMENT)");
        break;
        
    case OBJECT_TYPE_CLOSURE:
        SEE_FPRINTF(stream, " (CLOSURE)");
        break;
        
    case OBJECT_TYPE_CONTINUATION:
        SEE_FPRINTF(stream, " (CONTINUATION)");
        break;
        
    case OBJECT_TYPE_EXECUTION:
        SEE_FPRINTF(stream, " (EXECUTION)");
        break;
        
    case OBJECT_TYPE_EXTERNAL:
        SEE_FPRINTF(stream, " (EXTERNAL)");
        break;
        
    default:
        SEE_FPRINTF(stream, " (UNKNOWN)");
        break;
    }
}
#endif

static void
scan_ast(ast_node_t node, unsigned int *exps_count, unsigned *objs_count, unsigned *strs_count)
{
    ++ (*exps_count);
    
    switch (node->header.type)
    {
    case AST_SYMBOL:
    {
        if (node->symbol.type == SYMBOL_GENERAL)
        {
            /* Store a string for external load */
            if (node->header.priv == NULL)
                ++ (*strs_count);
        }
        else if (node->symbol.type == SYMBOL_STRING)
        {
            ++ (*objs_count);
        }
        else if (node->symbol.type == SYMBOL_NUMERIC)
        {
            /* Store a string for external constant */
            if (xstring_cstr(node->symbol.str)[0] == '#' && 
                internal_constant_match(node->symbol.str) == NULL)
                ++ (*strs_count);
        }
        
        break;
    }

    case AST_LAMBDA:
    {
        scan_ast(node->lambda.proc, exps_count, objs_count, strs_count);
        break;
    }

    case AST_WITH:
    {
        scan_ast(node->with.proc, exps_count, objs_count, strs_count);
        break;
    }


    case AST_PROC:
    {
        ast_node_t cur = node->proc.head;
        while (cur != NULL)
        {
            scan_ast(cur, exps_count, objs_count, strs_count);
            cur = cur->header.next;
            if (cur == node->proc.head) cur = NULL;
        }
        break;
    }

    case AST_AND:
    {
        ast_node_t cur = node->s_and.head;
        while (cur != NULL)
        {
            scan_ast(cur, exps_count, objs_count, strs_count);
            cur = cur->header.next;
            if (cur == node->s_and.head) cur = NULL;
        }
        break;
    }

    case AST_OR:
    {
        ast_node_t cur = node->s_or.head;
        while (cur != NULL)
        {
            scan_ast(cur, exps_count, objs_count, strs_count);
            cur = cur->header.next;
            if (cur == node->s_or.head) cur = NULL;
        }
        break;
    }


    case AST_APPLY:
    {
        int i;
        
        scan_ast(node->apply.func, exps_count, objs_count, strs_count);
        for (i = 0; i < node->apply.argc; ++ i)
            scan_ast(node->apply.args[i], exps_count, objs_count, strs_count);
        
        break;
    }

    case AST_COND:
    {
        scan_ast(node->cond.c, exps_count, objs_count, strs_count);
        scan_ast(node->cond.t, exps_count, objs_count, strs_count);
        if (node->cond.e != NULL)
            scan_ast(node->cond.e, exps_count, objs_count, strs_count);

        break;
    }
        
    case AST_SET:
    {
        if (node->header.priv == NULL)
        {
            /* Store a string for external store */
            ++ (*strs_count);
        }
        scan_ast(node->set.value, exps_count, objs_count, strs_count);
        break;
    }

    case AST_CALLCC:
    {
        scan_ast(node->callcc.node, exps_count, objs_count, strs_count);
        break;
    }

    default:
    {
        break;
    }
        
    }
}

typedef struct exp_handle_priv_s
{
    unsigned int exps_count;
    unsigned int objs_count;
    unsigned int strs_count;

    expression_t exps;
    object_t    *objs;
    xstring_t   *strs;
} *exp_handle_priv_t;

static void
handle_free(object_t object)
{
    exp_handle_priv_t priv = (exp_handle_priv_t)object->external.priv;
    SEE_FREE(priv->exps);
    SEE_FREE(priv->objs);
    int i;
    for (i = 0; i < priv->strs_count; ++ i)
        xstring_free(priv->strs[i]);
    SEE_FREE(priv->strs);
    SEE_FREE(priv);
}

static void
handle_enumerate(object_t object, void *q, void(*enqueue)(void *, object_t))
{
    exp_handle_priv_t priv = (exp_handle_priv_t)object->external.priv;
    int i;
    for (i = 0; i != priv->objs_count; ++ i)
    {
        enqueue(q, priv->objs[i]);
    }
}

static struct see_external_type_s handle_type = {
    .name      = "see exp handle",
    .enumerate = handle_enumerate,
    .free      = handle_free
};

/* simple routine to scan a integer in a string */
static int
scan_int(char *string, see_int_t *result)
{
    int sign;
    see_int_t temp;
    
    switch (*string)
    {
    case 0:
        *result = 0;
        return -1;

    case '-':
        sign = -1;
        ++ string;
        break;

    default:
        sign = 1;
        break;
    }

    temp = 0;
    while (*string >= '0' && *string <= '9')
    {
        temp = temp * 10 + (*string - '0');
        ++ string;
    }

    *result = temp * sign;
    return 0;
}

static expression_t 
expression_from_ast_internal(heap_t heap, ast_node_t node, object_t handle, exp_handle_priv_t priv, external_constant_match_f cm)
{
    expression_t result = priv->exps + (priv->exps_count ++);
    result->handle = handle;
    
    switch (node->header.type)
    {
    case AST_SYMBOL:
    {
        int type = node->symbol.type;
        switch (type)
        {
        case SYMBOL_NULL:
            result->type = EXP_TYPE_CONSTANT;
            result->depth = 1;
            result->constant.name = NULL;
            result->constant.value = OBJECT_NULL;
            break;
                
        case SYMBOL_GENERAL:
            result->depth = 1;
            if (node->header.priv == NULL)
            {
                result->type = EXP_TYPE_LOAD_EXTERNAL;
                result->load.name = priv->strs[priv->strs_count ++] = xstring_dup(node->symbol.str);
            }
            else
            {
                result->type = EXP_TYPE_LOAD;
                result->load.ref  = *(scope_ref_t)node->header.priv;
            }
            
            break;

        case SYMBOL_NUMERIC:
        {
            /* Now the #constant does spawn more than numeric constants */
            object_t o = NULL;
            
            if (xstring_cstr(node->symbol.str)[0] == '#')
            {
                o = internal_constant_match(node->symbol.str);
            }
            else
            {
                see_int_t v;
                scan_int(xstring_cstr(node->symbol.str), &v);
                o = INT_BOX(v);
            }
            
            result->depth = 1;
            if (o == NULL)
            {
                /* Throw an external escape if not found */
                result->constant.name = priv->strs[priv->strs_count ++] = xstring_dup(node->symbol.str);
                if (cm != NULL) result->constant.value = o = cm(xstring_cstr(node->symbol.str), xstring_len(node->symbol.str));
                result->type = o == NULL ? EXP_TYPE_CONSTANT_EXTERNAL : EXP_TYPE_CONSTANT;
            }
            else
            {
                result->type = EXP_TYPE_CONSTANT;
                result->constant.name = NULL;
                result->constant.value = o;
            }
            
            break;
        }

        case SYMBOL_STRING:
        {
            result->type = EXP_TYPE_CONSTANT;
            result->depth = 1;
            result->constant.name = NULL;
            result->constant.value = priv->objs[priv->objs_count ++];
            result->constant.value->string = xstring_dup(node->symbol.str);
            OBJECT_TYPE_INIT(result->constant.value, OBJECT_TYPE_STRING);
            break;
        }

        default:
            SEE_PRINTF_ERR("unknown type to translate symbol: %d\n", type);
            break;
            
        }

        break;
    }

    case AST_LAMBDA:
    {
        result->type = EXP_TYPE_CLOSURE;
        result->closure.argc = node->lambda.tail_list ? node->lambda.argc : node->lambda.argc + 1;
        /* XXX */
        result->closure.inherit = 1;
        
        result->closure.child = expression_from_ast_internal(heap, node->lambda.proc, handle, priv, cm);
        result->closure.child->parent = result;
        result->closure.child->next = NULL;
        result->depth = 1;
        
        break;
    }

    case AST_WITH:
    {
        result->type = EXP_TYPE_WITH;
        result->with.varc = node->with.varc;
        result->with.inherit = 1;
        
        result->with.child = expression_from_ast_internal(heap, node->with.proc, handle, priv, cm);
        result->with.child->parent = result;
        result->with.child->next = NULL;

        result->depth = result->with.child->depth;

        break;
    }


    case AST_PROC:
    {
        ast_node_t cur = node->proc.head;
        
        result->type = EXP_TYPE_PROC;
        result->proc.toplevel = node->proc.toplevel;
        expression_t e = result->proc.child =
            expression_from_ast_internal(heap, cur, handle, priv, cm);
        result->depth = e->depth;

        cur = cur->header.next;
        while (cur != node->proc.head)
        {
            e->parent = result;
            e->next = expression_from_ast_internal(heap, cur, handle, priv, cm);
            e = e->next;

            if (result->depth < e->depth)
                result->depth = e->depth;

            cur = cur->header.next;
        }
        e->parent = result;
        e->next = NULL;

        result->depth += node->proc.toplevel ? 1 : 0;
        break;
    }

    case AST_AND:
    {
        ast_node_t cur = node->s_and.head;
        
        result->type = EXP_TYPE_AND;
        expression_t e = result->and_exp.child =
            expression_from_ast_internal(heap, cur, handle, priv, cm);
        result->depth = e->depth;
        
        cur = cur->header.next;
        while (cur != node->s_and.head)
        {
            e->parent = result;
            e->next = expression_from_ast_internal(heap, cur, handle, priv, cm);
            e = e->next;

            if (result->depth < e->depth)
                result->depth = e->depth;

            cur = cur->header.next;
        }
        e->parent = result;
        e->next = NULL;

        break;
    }

    case AST_OR:
    {
        ast_node_t cur = node->s_or.head;
        
        result->type = EXP_TYPE_OR;
        expression_t e = result->or_exp.child =
            expression_from_ast_internal(heap, cur, handle, priv, cm);
        result->depth = e->depth;

        cur = cur->header.next;
        while (cur != node->s_or.head)
        {
            e->parent = result;
            e->next = expression_from_ast_internal(heap, cur, handle, priv, cm);
            e = e->next;

            if (result->depth < e->depth)
                result->depth = e->depth;

            cur = cur->header.next;
        }
        e->parent = result;
        e->next = NULL;

        break;
    }


    case AST_APPLY:
    {
        int i;
        
        result->type = EXP_TYPE_APPLY;
        result->apply.argc = node->apply.argc;
        result->apply.tail = node->apply.tail;
        expression_t e = result->apply.child = expression_from_ast_internal(heap, node->apply.func, handle, priv, cm);
        result->depth = e->depth;
        for (i = 0; i < node->apply.argc; ++ i)
        {
            e->parent = result;
            e->next = expression_from_ast_internal(heap, node->apply.args[i], handle, priv, cm);
            e = e->next;

            if (result->depth < e->depth + i + 1)
                result->depth = e->depth + i + 1;
        }
        e->parent = result;
        e->next = NULL;
        
        break;
    }

    case AST_COND:
    {
        result->type = EXP_TYPE_COND;
        
        result->cond.cd = expression_from_ast_internal(heap, node->cond.c, handle, priv, cm);
        result->cond.cd->parent = result;
        result->cond.cd->next = NULL;
        
        result->cond.th = expression_from_ast_internal(heap, node->cond.t, handle, priv, cm);
        result->cond.th->parent = result;
        result->cond.th->next = NULL;

        result->depth = result->cond.cd->depth > result->cond.th->depth ?
            result->cond.cd->depth : result->cond.th->depth;

        if (node->cond.e != NULL)
        {
            result->cond.el = expression_from_ast_internal(heap, node->cond.e, handle, priv, cm);
            result->cond.el->parent = result;
            result->cond.el->next = NULL;

            if (result->depth < result->cond.el->depth)
                result->depth = result->cond.el->depth;
        }
        else result->cond.el = NULL;

        break;
    }
        
    case AST_SET:
    {
        result->type = EXP_TYPE_STORE;
        result->store.exp = expression_from_ast_internal(heap, node->set.value, handle, priv, cm);
        result->store.exp->parent = result;
        result->store.exp->next = NULL;

        result->depth = result->store.exp->depth;

        if (node->header.priv == NULL)
        {
            result->type = EXP_TYPE_STORE_EXTERNAL;
            result->store.name = priv->strs[priv->strs_count ++] = xstring_dup(node->set.name);
        }
        else
        {
            result->store.ref = *(scope_ref_t)node->header.priv;
        }

        break;
    }

    case AST_CALLCC:
    {
        result->type = EXP_TYPE_CALLCC;
        result->callcc.tail = node->callcc.tail;
        result->callcc.exp = expression_from_ast_internal(heap, node->callcc.node, handle, priv, cm);
        result->callcc.exp->parent = result;
        result->callcc.exp->next = NULL;
        result->depth = result->callcc.exp->depth + 1;
        
        break;
    }

    default:
    {
        ERROR("unknown type to translate ast: %d\n", node->header.type);
        break;
    }
        
    }

    return result;
}

object_t
handle_from_ast(heap_t heap, ast_node_t node, external_constant_match_f cm)
{
    unsigned int exps_size = 0;
    unsigned int objs_size = 0;
    unsigned int strs_size = 0;

    scan_ast(node, &exps_size, &objs_size, &strs_size);

    object_t handle = heap_object_new(heap);
    if (handle == NULL) return NULL;
    
    expression_t exps = (expression_t)SEE_MALLOC(sizeof(struct expression_s) * exps_size);
    if (exps == NULL)
    {
        heap_unprotect(heap, handle);
        return NULL;
    }
    
    object_t *objs = (object_t *)SEE_MALLOC(sizeof(object_t) * objs_size);
    if (objs == NULL)
    {
        heap_unprotect(heap, handle);
        SEE_FREE(exps);
        return NULL;
    }

    xstring_t *strs = (xstring_t *)SEE_MALLOC(sizeof(xstring_t) * strs_size);
    if (strs == NULL)
    {
        heap_unprotect(heap, handle);
        SEE_FREE(exps);
        SEE_FREE(objs);
        return NULL;
    }
        

    int i;
    for (i = 0; i != objs_size; ++ i)
    {
        objs[i] = heap_object_new(heap);
        if (objs[i] == NULL) break;
    }

    if (i != objs_size)
    {
        int j;
        for (j = 0; j != i; ++ j)
            heap_unprotect(heap, objs[j]);
        heap_unprotect(heap, handle);
        SEE_FREE(objs);
        SEE_FREE(exps);
        SEE_FREE(strs);
        return NULL;
    }
    
    exp_handle_priv_t priv = (exp_handle_priv_t)SEE_MALLOC(sizeof(struct exp_handle_priv_s));
    if (priv == NULL)
    {
        for (i = 0; i != objs_size; ++ i)
            heap_unprotect(heap, objs[i]);
        heap_unprotect(heap, handle);
        SEE_FREE(objs);
        SEE_FREE(exps);
        SEE_FREE(strs);
        return NULL;
    }

    /* Now no allocation failure is possible */
    priv->exps = exps;
    priv->objs = objs;
    priv->strs = strs;
    priv->exps_count = 0;
    priv->objs_count = 0;
    priv->strs_count = 0;

    handle->external.type = &handle_type;
    handle->external.priv = priv;

    expression_t result = expression_from_ast_internal(heap, node, handle, priv, cm);
    result->parent = NULL;
    result->next = NULL;
    
    OBJECT_TYPE_INIT(handle, OBJECT_TYPE_EXTERNAL);

    for (i = 0; i < priv->objs_count; ++ i)
    {
        if (priv->objs[i] &&
            IS_OBJECT(priv->objs[i]))
        {
            heap_unprotect(heap, priv->objs[i]);
        }
    }

    return handle;
}

expression_t
handle_expression_get(object_t handle)
{
    return ((exp_handle_priv_t)(handle->external.priv))->exps;
}
