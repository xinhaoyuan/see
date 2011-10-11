#ifndef __SEE_OBJECT_H__
#define __SEE_OBJECT_H__

/* The common SEE object interface standard */

/* The types */
typedef struct heap_s       heap_s;
typedef struct object_s     object_s;
typedef struct slot_s       slot_s;
typedef struct execution_s  execution_s;
typedef struct scope_ref_s  scope_ref_s;
typedef struct expression_s expression_s;

typedef heap_s       *heap_t;
typedef object_s     *object_t;
typedef slot_s       *slot_t;
typedef execution_s  *execution_t;
typedef scope_ref_s  *scope_ref_t;
typedef expression_s *expression_t;

/* The encoding suffix for distinguish object */
#define ENCODE_SUFFIX_OBJECT 0
#define ENCODE_SUFFIX_SYMBOL 1
#define ENCODE_SUFFIX_INT    2
#define ENCODE_SUFFIX_BOXED  3

#define OBJECT_NULL          ((object_t)0x1)

/* Assume that the object pointer is at least aligned by 2 bits */
#define ENCODE_SUFFIX(object) ((unsigned int)(object) & 0x3)

/* The object type id */
#define OBJECT_TYPE_UNINITIALIZED 0

#define OBJECT_TYPE_VALID         4
#define OBJECT_TYPE_STRING        4
#define OBJECT_TYPE_PAIR          5
#define OBJECT_TYPE_VECTOR        6
#define OBJECT_TYPE_ENVIRONMENT   7
#define OBJECT_TYPE_CLOSURE       8
#define OBJECT_TYPE_CONTINUATION  9
#define OBJECT_TYPE_EXECUTION     10
#define OBJECT_TYPE_EXTERNAL      11

#include "object_impl.h"

#ifndef SLOT_INIT
void slot_init(slot_t slot, object_t object);
#define SLOT_INIT(slot) slot_init(&(slot), object)
#endif

#ifndef SLOT_GET
object_t slot_get(slot_t slot);
#define SLOT_GET(slot) (slot_get(&(slot)))
#endif

#ifndef SLOT_SET
void slot_set(slot_t slot, object_t object);
#define SLOT_SET(slot) slot_set(&(slot), object)
#endif

#ifndef OBJECT_TYPE
int object_type(object_t object);
#define OBJECT_TYPE(object) object_type(object)
#endif

#ifndef OBJECT_TYPE_INIT
void object_type_init(object_t object, int type);
#define OBJECT_TYPE_INIT(object, type) object_type_init(object, type)
#endif


struct object_s
{
	union
	{
		xstring_t string;

		struct
		{
			slot_s slot_car, slot_cdr;
		} pair;

		struct
		{
			unsigned int length;
			slot_s      *slot_entry;
		} vector;

		struct
		{
			unsigned int length;
			slot_s      *slot_entry;
			object_t     parent;
		} environment;

		struct
		{
			expression_t exp;
			unsigned int argc;
			object_t     env;
		} closure;

		struct
		{
			expression_t exp;
			object_t     env;

			unsigned int stack_count;
			object_t    *stack;
		} continuation;

		struct
		{
			void *priv;
			
			void(*enumerate)(object_t, void *, void(*)(void *, object_t));
			void(*free)(object_t);
		} external;
	};
};

struct scope_ref_s
{
	unsigned int parent_level;
	unsigned int offset;
};

#define EXP_TYPE_APPLY   0x00
#define EXP_TYPE_PROC    0x01
#define EXP_TYPE_AND     0x02
#define EXP_TYPE_OR      0x03
#define EXP_TYPE_COND    0x04
#define EXP_TYPE_SET     0x05
#define EXP_TYPE_REF     0x06
#define EXP_TYPE_VALUE   0x07
#define EXP_TYPE_CLOSURE 0x08
#define EXP_TYPE_WITH    0x09
#define EXP_TYPE_CALLCC  0x0a

struct expression_s
{
	object_t handle;
	unsigned int type;
	
	expression_t parent;
	expression_t next;
	
	union
	{
		struct
		{
			int          tail;
			unsigned int argc;
			expression_t child;
		} apply;
		
		struct
		{
			expression_t child;
		} proc, and_exp, or_exp;

		struct
		{
			expression_t cd, th, el;
		} cond;

		struct
		{
			int tail;
			expression_t exp;
		} callcc;

		struct
		{
			unsigned int argc;
			int          inherit;
			expression_t child;
		} closure;

		struct
		{
			unsigned int varc;
			int inherit;
			expression_t child;
		} with;

		struct
		{
			struct scope_ref_s ref;
			expression_t exp;
		} set;

		struct scope_ref_s ref;
		
		object_t value;
	};
};

struct execution_s
{
	expression_t  exp;
	object_t      env;

	unsigned int stack_alloc;
	unsigned int stack_count;
	object_t    *stack;

	object_t value;
	int      to_push;
};

#define INT_UNBOX(object)      ((int)(object) >> 2)
#define INT_BOX(i)             ((object_t)(((i) << 2) | ENCODE_SUFFIX_INT))

#define EXTERNAL_UNBOX(object) ((void *)((unsigned int)(object) & ~(unsigned int)0x3))
#define EXTERNAL_BOX(e)        ((object_t)((unsigned int)(e) | ENCODE_SUFFIX_BOXED))

#define IS_OBJECT(object)      (ENCODE_SUFFIX(object) == ENCODE_SUFFIX_OBJECT)
#define IS_INT(object)         (ENCODE_SUFFIX(object) == ENCODE_SUFFIX_INT)
#define IS_SYMBOL(object)      (ENCODE_SUFFIX(object) == ENCODE_SUFFIX_SYMBOL)
#define IS_EXTERNAL(object)    (ENCODE_SUFFIX(object) == ENCODE_SUFFIX_BOXED)

heap_t   heap_new(void);
void     heap_free(heap_t heap);
object_t heap_object_new(heap_t heap);

void heap_protect_from_gc(heap_t heap, object_t object);
void heap_unprotect(heap_t heap, object_t object);
void heap_detach(object_t object);

execution_t heap_execution_new(heap_t heap);
void        heap_execution_free(execution_t ex);

object_t continuation_from_expression(heap_t heap, expression_t e);

#endif
