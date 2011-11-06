#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* All header files that a interpreter may need */
#include "object.h"
#include "as/simple_parse.h"
#include "as/syntax_parse.h"
#include "as/free.h"
#include "as/dump.h"
#include "vm/symref.h"
#include "vm/io.h"
#include "vm/vm.h"

/* This source file shows the usage of SEE interfaces */

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
	if (argc != 3)
	{
		printf("USAGE: %s [i|s|r] file\n", args[0]);
		return -1;
	}
	else
	{
		char mode = args[1][0];
	
		struct simple_stream s;
		s.file = fopen(args[2], "r");
		s.buf  = BUF_EMPTY;

		/* Parse input stream into general AST node */
		ast_node_t n = ast_simple_parse_char_stream((stream_in_f)simple_stream_in, &s);

		fclose(s.file);

		if (mode == 'i')
		{
			ast_dump(n, stdout);
			ast_free(n);
			return 0;
		}
		
		/* Parse the AST symbol */
		ast_syntax_parse(n, 0);
		sematic_symref_analyse(n);

		if (mode == 's')
		{
			ast_dump(n, stdout);
			ast_free(n);
			return 0;
		}
		else if (mode != 'r')
		{
			printf("ERROR OPTION\n");
			return -1;
		}

		heap_t heap = heap_new();
		/* Generate the object which encapsulate the compact AST
		 * format (expression) to execute */
		object_t handle = handle_from_ast(heap, n);
		/* release the ast */
		ast_free(n);

		/* Construct the SEE object */
		object_t prog = continuation_from_handle(heap, handle);
				
		execution_t ex = NULL;
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
			/* An example for handling external calls: display */
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

					ex->value = OBJECT_NULL;
				}
				else ex->value = OBJECT_NULL;
			}
		}

		// printf("STACK_COUNT %d\n", ex.stack_count);

		heap_free(heap);
		return 0;
	}
}