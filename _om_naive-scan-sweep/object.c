#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"

/* The definition of heap structure is hidden to user, so we can
 * change the internal implementation unconspicuously */

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
	switch (object->gc.type)
	{
	case OBJECT_STRING:
		xstring_free(object->string);
		break;

	case OBJECT_VECTOR:
		free(object->vector.slot_entry);
		break;

	case OBJECT_ENVIRONMENT:
		free(object->environment.slot_entry);
		break;

	case OBJECT_CONTINUATION:
		free(object->continuation.stack);
		break;

	case OBJECT_EXECUTION:
		break;

	case OBJECT_EXTERNAL:
		object->external.free(object);
		break;
	}

	free(object);
}

heap_t
heap_new(void)
{
	heap_t result = (heap_t)malloc(sizeof(struct heap_s));

	/* Initialize the gc header list */
	result->managed.prev =
		result->managed.next = &result->managed;

	result->locked.prev =
		result->locked.next = &result->locked;

	/* Initialize counters */
	result->count = 0;
	result->threshold = 1024;

	return result;
}

void
heap_free(heap_t heap)
{
	object_t cur;
	
	/* Ignore all locked object since they may be hold by external
	 * prog */

#if 0
	cur = (object_t)heap->locked.next;
	while (cur != (object_t)&heap->locked)
	{
		object_t to_free = cur;
		cur = (object_t)cur->gc.next;
		
		object_free(to_free);
	}
#endif

	cur = (object_t)heap->managed.next;
	while (cur != (object_t)&heap->managed)
	{
		object_t to_free = cur;
		cur = (object_t)cur->gc.next;
		
		object_free(to_free);
	}

	free(heap);
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
	if (is_object(o) && o && o->gc.mark == 0)
	{
		o->gc.mark = 1;
		
		q->queue[q->head ++] = o;
		q->head %= q->alloc;
		if (q->head == q->tail)
		{
			q->queue = (object_t *)realloc(q->queue, sizeof(object_t) * (q->alloc << 1));
			memcpy(q->queue + q->alloc, q->queue, sizeof(object_t) * q->head);
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
	q.queue = (object_t *)malloc(sizeof(object_t) * q.alloc);

	object_t now = (object_t)heap->locked.next;
	while (now != (object_t)&heap->locked)
	{
		now->gc.mark = 0;
		exqueue_enqueue(&q, now);
		now = (object_t)now->gc.next;
	}

	while (q.head != q.tail)
	{
		now = q.queue[q.tail ++];
		q.tail %= q.alloc;

		/* Enumerate all links inside a object */
		switch (now->gc.type)
		{
		case OBJECT_PAIR:
			exqueue_enqueue(&q, SLOT_GET(now->pair.slot_car));
			exqueue_enqueue(&q, SLOT_GET(now->pair.slot_cdr));
			break;

		case OBJECT_VECTOR:
		{
			int i;
			for (i = 0; i != now->vector.length; ++ i)
			{
				exqueue_enqueue(&q, SLOT_GET(now->vector.slot_entry[i]));
			}
			break;
		}

		case OBJECT_ENVIRONMENT:
		{
			int i;
			for (i = 0; i != now->environment.length; ++ i)
			{
				exqueue_enqueue(&q, SLOT_GET(now->environment.slot_entry[i]));
			}
			exqueue_enqueue(&q, now->environment.parent);
			break;
		}

		case OBJECT_CLOSURE:
			if (now->closure.exp != NULL)
				exqueue_enqueue(&q, now->closure.exp->handle);
			exqueue_enqueue(&q, now->closure.env);
			break;

		case OBJECT_CONTINUATION:
		{
			int i;
			for (i = 0; i != now->continuation.stack_count; ++ i)
			{
				exqueue_enqueue(&q, now->continuation.stack[i]);
			}
			if (now->continuation.exp != NULL)
				exqueue_enqueue(&q, now->continuation.exp->handle);
			exqueue_enqueue(&q, now->continuation.env);
			break;
		}
		
		case OBJECT_EXECUTION:
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
		
		case OBJECT_EXTERNAL:
		{
			now->external.enumerate(now, &q, (void(*)(void *, object_t))&exqueue_enqueue);
			break;
		}

		}
	}

	now = (object_t)heap->managed.next;
	while (now != (object_t)&heap->managed)
	{
		if (now->gc.mark)
		{
			now->gc.mark = 0;
			now = (object_t)now->gc.next;
		}
		else
		{
			now->gc.prev->next = now->gc.next;
			now->gc.next->prev = now->gc.prev;

			object_t to_free = now;
			now = (object_t)now->gc.next;
			
			object_free(to_free);
			
			-- heap->count;
		}
		
	}
	
	return;
}

object_t
heap_object_new(heap_t heap)
{
	if (heap->count >= heap->threshold)
	{
		do_gc(heap);
		/* Hardcoded rule for recalculation */
		heap->threshold = (heap->count + 1024) << 1;

#if GC_DEBUG
		fprintf(stderr, "DEBUG: gc object count: %d\n", heap->count);
#endif
	}

	++ heap->count;
	
	object_t result =(object_t)malloc(sizeof(struct object_s));
	result->gc.type = OBJECT_UNINITIALIZED;
	result->gc.mark = 0;
	
	result->gc.next = &heap->locked;
	result->gc.prev = heap->locked.prev;
	result->gc.prev->next = &result->gc;
	result->gc.next->prev = &result->gc;
	
	return result;
}

void
heap_protect_from_gc(heap_t heap, object_t object)
{
	object->gc.prev->next = object->gc.next;
	object->gc.next->prev = object->gc.prev;
	

	object->gc.next = &heap->locked;
	object->gc.prev = heap->locked.prev;
	object->gc.prev->next = &object->gc;
	object->gc.next->prev = &object->gc;
}

void
heap_unprotect(heap_t heap, object_t object)
{
	object->gc.mark = 0;

	object->gc.prev->next = object->gc.next;
	object->gc.next->prev = object->gc.prev;
	

	object->gc.next = &heap->managed;
	object->gc.prev = heap->managed.prev;
	object->gc.prev->next = &object->gc;
	object->gc.next->prev = &object->gc;
}

void
heap_detach(object_t object)
{
	object->gc.prev->next = object->gc.next;
	object->gc.next->prev = object->gc.prev;
}

object_t
continuation_from_expression(heap_t heap, expression_t e)
{
	object_t prog = heap_object_new(heap);
	prog->continuation.exp = e;
	prog->continuation.env = NULL;
	prog->continuation.stack_count = 0;
	prog->continuation.stack = NULL;
	prog->gc.type = OBJECT_CONTINUATION;
	
	return prog;
}
