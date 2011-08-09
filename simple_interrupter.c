#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <object.h>

#include "as/simple_parse.h"
#include "as/syntax_parse.h"
#include "vm/symref.h"
#include "vm/io.h"
#include "vm/vm.h"

struct simple_stream
{
	FILE *file;
	int   buf;
};

#define BUF_EMPTY (-2)

int simple_stream_in(struct simple_stream *stream, int advance)
{
	int r;
	if (advance)
	{
		if (stream->buf == BUF_EMPTY)
			r = fgetc(stream->file);
		else
		{
			r = stream->buf;
			stream->buf = BUF_EMPTY;
		}
	}
	else
	{
		if (stream->buf == BUF_EMPTY)
		{
			stream->buf = fgetc(stream->file);
			if (stream->buf < 0) stream->buf = -1;
		}
		
		r = stream->buf;
	}
	return r;
}

int main(int argc, const char *args[])
{
	if (argc != 2)
	{
		printf("USAGE: %s file\n", args[0]);
		return -1;
	}
	else
	{
		heap_t heap = heap_new();
	
		struct simple_stream s;
		s.file = fopen(args[1], "r");
		s.buf  = BUF_EMPTY;
		
		ast_node_t n = ast_simple_parse_char_stream((stream_in_f)simple_stream_in, &s);

		fclose(s.file);
		
		n = ast_syntax_parse(n, 0);
		sematic_symref_analyse(n);

		expression_t e = expression_from_ast(heap, n);
		object_t prog = continuation_from_expression(heap, e);
				
		struct execution_s ex;
		object_t  ex_func;
		int       ex_argc;
		object_t *ex_args;
		
		object_t ret;
		while (1)
		{
			int r = vm_apply(heap, prog, 0, NULL, &ret, &ex, &ex_func, &ex_argc, &ex_args);
			prog = NULL;
			if (r == APPLY_EXIT || r == APPLY_EXIT_NO_VALUE)
				break;
			/* An example for handling external calls */
			if (r == APPLY_EXTERNAL_CALL)
			{
				if (xstring_equal_cstr(ex_func->string, "display", -1))
				{
					int i;
					for (i = 0; i != ex_argc; ++ i)
					{
						object_dump(ex_args[i]);
					}
					printf("\n");

					ex.value = NULL;
				}
				else ex.value = NULL;
			}
		}

		// printf("STACK_COUNT %d\n", ex.stack_count);

		heap_free(heap);
		return 0;
	}
}
