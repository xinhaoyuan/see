#include "../config.h"
#include "../object.h"
#include "../vm/io.h"
#include "../lib/xstring.h"


#define GC_PARAM_0 10240
#define GC_PARAM_1 10240

/* The definition of heap structure is hidden to user, so we can
 * change the internal implementation unconspicuously */

#define TO_GC(o)      ((gc_header_t)(o) - 1)
#define TO_OBJECT(gc) ((object_t)((gc_header_t)(gc) + 1))

/* No magic check anymore */
// static char __sa[sizeof(gc_header_s) == GC_HEADER_SPACE ? 0 : -1] __attribute__((unused));

struct heap_s
{
	unsigned int count;			/* living object count */
	unsigned int threshold;		/* count trigger for collecting */
	/* lists for managed objects and hold objects(root objects) */
	struct gc_header_s managed, locked;
};

static inline void
object_free(object_t object)
{	
	switch (OBJECT_TYPE(object))
	{		
	case OBJECT_TYPE_STRING:
		xstring_free(object->string);
		break;

	case OBJECT_TYPE_VECTOR:
		SEE_FREE(object->vector.slot_entry);
		break;

	case OBJECT_TYPE_ENVIRONMENT:
		SEE_FREE(object->environment.slot_entry);
		break;

	case OBJECT_TYPE_CONTINUATION:
		SEE_FREE(object->continuation.stack);
		break;

	case OBJECT_TYPE_EXTERNAL:
		if (object->external.type && object->external.type->free)
			object->external.type->free(object);
		break;
	}

	SEE_FREE(TO_GC(object));
}

heap_t
heap_new(void)
{
	heap_t result = (heap_t)SEE_MALLOC(sizeof(struct heap_s));

	/* Initialize the gc header list */
	result->managed.prev =
		result->managed.next = &result->managed;

	result->locked.prev =
		result->locked.next = &result->locked;

	/* Initialize counters */
	result->count = 0;
	result->threshold = GC_PARAM_0;

	return result;
}

void
heap_free(heap_t heap)
{
	gc_header_t cur_gc;
	
	/* Need to ignore all locked object since they may be hold by external
	 * prog ??? */
	/* Currently no */
	
#if 1
	cur_gc = heap->locked.next;
	while (cur_gc != &heap->locked)
	{
		gc_header_t last_gc = cur_gc;
		cur_gc = cur_gc->next;
		
		object_free(TO_OBJECT(last_gc));
	}
#endif

	cur_gc = heap->managed.next;
	while (cur_gc != &heap->managed)
	{
		gc_header_t last_gc = cur_gc;
		cur_gc = cur_gc->next;
		
		object_free(TO_OBJECT(last_gc));
	}

	SEE_FREE(heap);
}

typedef struct exqueue_s
{
	unsigned int alloc;
	unsigned int head;
	unsigned int tail;

	object_t *queue;
} exqueue_s;

static void
exqueue_enqueue(exqueue_s *q, object_t o)
{
	if (IS_OBJECT(o) && TO_GC(o)->mark == 0)
	{
		TO_GC(o)->mark = 1;
		
		q->queue[q->head ++] = o;
		q->head %= q->alloc;
		if (q->head == q->tail)
		{
			q->queue = (object_t *)SEE_REALLOC(q->queue, sizeof(object_t) * (q->alloc << 1));
			SEE_MEMCPY(q->queue + q->alloc, q->queue, sizeof(object_t) * q->head);
			q->head += q->alloc;
			q->alloc <<= 1;
		}
	}
}
	
static void
do_gc(heap_t heap)
{
	exqueue_s q;
	q.alloc = 256;
	q.head  = 0;
	q.tail  = 0;
	q.queue = (object_t *)SEE_MALLOC(sizeof(object_t) * q.alloc);

	gc_header_t cur_gc = heap->locked.next;
	while (cur_gc != &heap->locked)
	{
		cur_gc->mark = 0;
		exqueue_enqueue(&q, TO_OBJECT(cur_gc));
		cur_gc = cur_gc->next;
	}

	object_t now;
	while (q.head != q.tail)
	{
		now = q.queue[q.tail ++];
		q.tail %= q.alloc;

		/* Enumerate all links inside a object */
		switch (OBJECT_TYPE(now))
		{
		case OBJECT_TYPE_PAIR:
			exqueue_enqueue(&q, SLOT_GET(now->pair.slot_car));
			exqueue_enqueue(&q, SLOT_GET(now->pair.slot_cdr));
			break;

		case OBJECT_TYPE_VECTOR:
		{
			int i;
			for (i = 0; i != now->vector.length; ++ i)
			{
				exqueue_enqueue(&q, SLOT_GET(now->vector.slot_entry[i]));
			}
			break;
		}

		case OBJECT_TYPE_ENVIRONMENT:
		{
			int i;
			for (i = 0; i != now->environment.length; ++ i)
			{
				exqueue_enqueue(&q, SLOT_GET(now->environment.slot_entry[i]));
			}
			exqueue_enqueue(&q, now->environment.parent);
			break;
		}

		case OBJECT_TYPE_CLOSURE:
			if (now->closure.exp != NULL)
				exqueue_enqueue(&q, now->closure.exp->handle);
			exqueue_enqueue(&q, now->closure.env);
			break;

		case OBJECT_TYPE_CONTINUATION:
		{
			int i;
			for (i = 0; i != now->continuation.stack_count - 1; ++ i)
			{
				exqueue_enqueue(&q, now->continuation.stack[i]);
			}
			if (now->continuation.exp != NULL)
				exqueue_enqueue(&q, now->continuation.exp->handle);
			exqueue_enqueue(&q, now->continuation.env);
			break;
		}
		
		case OBJECT_TYPE_EXECUTION:
		{
			execution_t ex = (execution_t)now;
			if (ex->exp != NULL)
				exqueue_enqueue(&q, ex->exp->handle);

			exqueue_enqueue(&q, ex->env);
			exqueue_enqueue(&q, ex->value);
			
			int i;
			for (i = 0; i != ex->stack_count; ++ i)
			{
				exqueue_enqueue(&q, ex->stack[i]);
			}
			break;
		}
		
		case OBJECT_TYPE_EXTERNAL:
		{
			if (now->external.type && now->external.type->enumerate)
				now->external.type->enumerate(now, &q, (void(*)(void *, object_t))&exqueue_enqueue);
			break;
		}

		}
	}

	cur_gc = heap->managed.next;
	while (cur_gc != &heap->managed)
	{
		if (cur_gc->mark)
		{
			cur_gc->mark = 0;
			cur_gc = cur_gc->next;
		}
		else
		{
			cur_gc->prev->next = cur_gc->next;
			cur_gc->next->prev = cur_gc->prev;

			gc_header_t last_gc = cur_gc;
			cur_gc = cur_gc->next;
			
			object_free(TO_OBJECT(last_gc));
			
			-- heap->count;
		}
	}

	if (q.queue) SEE_FREE(q.queue);
	return;
}

object_t
heap_object_new(heap_t heap)
{
	if (heap->count >= heap->threshold)
	{
		do_gc(heap);
		/* Hardcoded rule for recalculation */
		heap->threshold = (heap->count + GC_PARAM_1) << 1;
		WARNING("DEBUG: gc object count: %d\n", heap->count);
	}

	++ heap->count;
	
	gc_header_t result = (gc_header_t)SEE_MALLOC(sizeof(gc_header_s) + sizeof(object_s));
	result->type = OBJECT_TYPE_UNINITIALIZED;
	result->mark = 0;
	
	result->prot_level = 1;
	result->next = &heap->locked;
	result->prev = heap->locked.prev;
	result->prev->next = result;
	result->next->prev = result;
	
	return TO_OBJECT(result);
}

void
heap_protect_from_gc(heap_t heap, object_t object)
{
	gc_header_t gc = TO_GC(object);
	++ gc->prot_level;
	if (gc->prot_level == 1)
	{
		gc->prev->next = gc->next;
		gc->next->prev = gc->prev;
		
		gc->next = &heap->locked;
		gc->prev = heap->locked.prev;
		gc->prev->next = gc;
		gc->next->prev = gc;
	}
}

void
heap_unprotect(heap_t heap, object_t object)
{
	gc_header_t gc = TO_GC(object);
	if (gc->prot_level > 0)
	{
		-- gc->prot_level;

		if (gc->prot_level == 0)
		{
			gc->mark = 0;
			gc->prev->next = gc->next;
			gc->next->prev = gc->prev;
			
			
			gc->next = &heap->managed;
			gc->prev = heap->managed.prev;
			gc->prev->next = gc;
			gc->next->prev = gc;
		}
	}
}

void
heap_detach(object_t object)
{
	gc_header_t gc = TO_GC(object);
	gc->prev->next = gc->next;
	gc->next->prev = gc->prev;
}

object_t
continuation_from_handle(heap_t heap, object_t handle)
{
	object_t prog = heap_object_new(heap);
	prog->continuation.exp = handle_expression_get(handle);
	prog->continuation.env = OBJECT_NULL;
	prog->continuation.stack_count = 1;
	prog->continuation.stack = (object_t *)SEE_MALLOC(sizeof(object_t));
	prog->continuation.stack[0] = (object_t)((see_uint_t)(prog->continuation.exp->depth));
	OBJECT_TYPE_INIT(prog, OBJECT_TYPE_CONTINUATION);

	return prog;
}

execution_t
heap_execution_new(heap_t heap)
{
	gc_header_t gc = (gc_header_t)SEE_MALLOC(sizeof(gc_header_s) + sizeof(execution_s));
	execution_t ex = (execution_t)(gc + 1);

	gc->prot_level = 0;
	gc->mark = 0;
	gc->prev =
		gc->next = gc;
	heap_protect_from_gc(heap, (object_t)ex);
			
	ex->exp = NULL;
	ex->env = OBJECT_NULL;
			
	ex->stack_count = 0;
	ex->stack_alloc = 1;
	ex->stack = (object_t *)SEE_MALLOC(sizeof(object_t));

	return ex;
}

object_t
continuation_from_execution(heap_t heap, execution_t ex)
{
	object_t cont = heap_object_new(heap);
	if (cont == NULL) return NULL;

	object_t *stack = (object_t *)SEE_MALLOC(sizeof(object_t) * (ex->stack_count + 1));
	if (stack == NULL)
	{
		heap_object_free(heap, cont);
		return NULL;
	}
					
	cont->continuation.exp = ex->exp->parent;
	cont->continuation.env = ex->env;
	cont->continuation.stack_count = ex->stack_count + 1;
	cont->continuation.stack = stack;
	SEE_MEMCPY(cont->continuation.stack, ex->stack, sizeof(object_t) * ex->stack_count);
	cont->continuation.stack[ex->stack_count] = (object_t)((see_uint_t)ex->stack_alloc);
	OBJECT_TYPE_INIT(cont, OBJECT_TYPE_CONTINUATION);

	return cont;
}

void
heap_execution_free(execution_t ex)
{
	SEE_FREE(ex->stack);
	heap_detach((object_t)ex);
	SEE_FREE(TO_GC(ex));
}

void
heap_object_free(heap_t heap, object_t object)
{
	heap_unprotect(heap, object);
}
