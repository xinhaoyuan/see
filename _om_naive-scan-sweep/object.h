#ifndef __SEE_OBJECT_H__
#define __SEE_OBJECT_H__

#include <xstring.h>

/* Internal datas for tracking all objects of a scope */
typedef struct heap_s *heap_t;

typedef struct object_s     object_s;
typedef struct slot_s       slot_s; /* Simply wrap the slot to object pointer */
typedef struct gc_header_s  gc_header_s;
typedef struct execution_s  execution_s;
typedef struct scope_ref_s  scope_ref_s;
typedef struct expression_s expression_s;

typedef object_s     *object_t;
typedef slot_s       *slot_t;
typedef gc_header_s  *gc_header_t;
typedef execution_s  *execution_t;
typedef scope_ref_s  *scope_ref_t;
typedef expression_s *expression_t;

struct slot_s
{
	object_t _value;
};

#define SLOT_GET(slot)         (slot)._value
#define SLOT_SET(slot, value)  (slot)._value = (value)
#define SLOT_INIT(slot, value) (slot)._value = (value)

/* The encoding suffix for distinguish object */
#define ENCODE_SUFFIX_OBJECT 0
#define ENCODE_SUFFIX_SYMBOL 1
#define ENCODE_SUFFIX_INT    2
#define ENCODE_SUFFIX_BOXED  3

#define OBJECT_UNINITIALIZED 0
#define OBJECT_STRING        1
#define OBJECT_PAIR          2
#define OBJECT_VECTOR        3
#define OBJECT_ENVIRONMENT   4
#define OBJECT_CLOSURE       5
#define OBJECT_CONTINUATION  6
#define OBJECT_EXECUTION     7
#define OBJECT_EXTERNAL      8

struct gc_header_s
{
	unsigned char type;
	unsigned char mark;
	gc_header_t   next, prev;
};

struct object_s
{
	struct gc_header_s gc;

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

struct execution_s
{
	struct gc_header_s gc;
	
	expression_t  exp;
	object_t      env;

	unsigned int stack_alloc;
	unsigned int stack_count;
	object_t    *stack;

	object_t value;
	int      to_push;
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

struct scope_ref_s
{
	unsigned int parent_level;
	unsigned int offset;
};

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
		} proc, and, or;

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

#define encode_suffix(i)       ((unsigned int)(i) & 0x3)

#define int_unbox(object)      ((int)(object) >> 2)
#define int_box(i)             ((object_t)(((i) << 2) | ENCODE_SUFFIX_INT))

#define external_unbox(object) ((void *)((unsigned int)(object) & ~(unsigned int)0x3))
#define external_box(e)        ((object_t)((unsigned int)(e) | ENCODE_SUFFIX_BOXED))

#define is_object(object)      (encode_suffix(object) == ENCODE_SUFFIX_OBJECT)
#define is_int(object)         (encode_suffix(object) == ENCODE_SUFFIX_INT)
#define is_symbol(object)      (encode_suffix(object) == ENCODE_SUFFIX_SYMBOL)
#define is_external(object)    (encode_suffix(object) == ENCODE_SUFFIX_BOXED)

heap_t   heap_new(void);
void     heap_free(heap_t heap);
object_t heap_object_new(heap_t heap);

void heap_protect_from_gc(heap_t heap, object_t object);
void heap_unprotect(heap_t heap, object_t object);
void heap_detach(object_t object);

object_t continuation_from_expression(heap_t heap, expression_t e);

#endif
