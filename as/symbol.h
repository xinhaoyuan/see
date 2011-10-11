#ifndef __SEE_AS_SYMBOL_H__
#define __SEE_AS_SYMBOL_H__

#include "../lib/xstring.h"

#define SYMBOL_NULL    0
#define SYMBOL_GENERAL 1
#define SYMBOL_NUMERIC 2
#define SYMBOL_STRING  3

typedef struct as_symbol_s
{
	int       type;
	xstring_t str;
} *as_symbol_t;

#endif
