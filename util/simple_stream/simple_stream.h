#ifndef __SEE_UTIL_SIMPLE_STREAM_H__
#define __SEE_UTIL_SIMPLE_STREAM_H__

#include <stdio.h>

struct simple_stream_s
{
    FILE *file;
    int   buf;
};

typedef struct simple_stream_s  simple_stream_s;
typedef struct simple_stream_s *simple_stream_t;

void simple_stream_open(simple_stream_t stream, FILE *file);
void simple_stream_close(simple_stream_t stream);
int  simple_stream_in(simple_stream_t stream, int advance);

#endif
