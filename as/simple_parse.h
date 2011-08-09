#ifndef __SEE_AS_SIMPLE_PARSE_H__
#define __SEE_AS_SIMPLE_PARSE_H__

#include "ast.h"

typedef int(*stream_in_f)(void*,int);
ast_node_t ast_simple_parse_char_stream(stream_in_f stream_in, void *stream);

#endif
