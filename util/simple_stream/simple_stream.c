#include "simple_stream.h"

#define BUF_END   (-1)
#define BUF_ERROR (-2)
#define BUF_EMPTY (-3)

void
simple_stream_open(simple_stream_t stream, FILE *file)
{
    stream->file = file;
    stream->buf  = file == NULL ? BUF_ERROR : BUF_EMPTY;
}

void
simple_stream_close(simple_stream_t stream)
{
    if (stream->file != NULL) fclose(stream->file);
    stream->file = NULL;
    stream->buf  = BUF_ERROR;
}

int
simple_stream_in(simple_stream_t stream, int advance)
{
    int r;
    if (advance)
    {
        if (stream->buf == BUF_EMPTY)
            r = fgetc(stream->file);
        else
        {
            r = stream->buf;
            if (stream->buf != BUF_ERROR)
                stream->buf = BUF_EMPTY;
        }
    }
    else
    {
        if (stream->buf == BUF_EMPTY)
        {
            stream->buf = fgetc(stream->file);
            if (stream->buf < 0) stream->buf = BUF_END;
        }
        
        r = stream->buf;
    }
    return r;
}
