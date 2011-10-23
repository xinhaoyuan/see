#ifndef __SEE_OBJECT_IMPL_H__
#define __SEE_OBJECT_IMPL_H__

#include "../lib/xstring.h"

typedef struct gc_header_s  gc_header_s;
typedef gc_header_s        *gc_header_t;

struct slot_s
{
	object_t _value;
};

#define SLOT_GET(slot)         (slot)._value
#define SLOT_SET(slot, value)  (slot)._value = (value)
#define SLOT_INIT(slot, value) (slot)._value = (value)
#define OBJECT_TYPE(object)												\
	({																	\
		unsigned int suf = ENCODE_SUFFIX(object);						\
		suf == ENCODE_SUFFIX_OBJECT ? (((gc_header_t)(object) - 1)->type) : suf; \
	})
#define OBJECT_TYPE_INIT(o, t)			\
	if (IS_OBJECT(o)) ((gc_header_t)(o) - 1)->type = t

/* GC_HEADER_SPACE specify the size of gc_header_s. MAGIC */
#define GC_HEADER_SPACE 24

struct gc_header_s
{
	union
	{
		struct
		{
			unsigned char type;
			unsigned char mark;
			unsigned int  prot_level;
			gc_header_t   next, prev;
		};

		char __space[GC_HEADER_SPACE];
	};
};

#endif
