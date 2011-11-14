#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm/interp.h"

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

	char mode = args[1][0];
	struct simple_stream s;
	ast_node_t n;
	interp_s __interp;
	interp_t interp = &__interp;
	object_t prog;
	
	switch (mode)
	{
	case 'i':
	case 's':

		s.file = fopen(args[2], "r");
		s.buf  = BUF_EMPTY;

		n = ast_simple_parse_char_stream((stream_in_f)simple_stream_in, &s);

		fclose(s.file);

		if (mode == 'i')
		{
			ast_dump(n, stdout);
			ast_free(n);
			
			break;
		}

		ast_syntax_parse(n, 0);
		sematic_symref_analyse(n);

		ast_dump(n, stdout);
		ast_free(n);

		break;

	case 'r':

		interp_initialize(interp, 16);
		
		s.file = fopen(args[2], "r");
		s.buf  = BUF_EMPTY;

		prog = interp_eval(interp, (stream_in_f)simple_stream_in, &s);

		fclose(s.file);

		interp_apply(interp, prog, 0, NULL);
		
		int       ex_argc = 0;
		object_t *ex_args;
		object_t  ex_ret = NULL;
		int i;
		
		while (1)
		{
			int r = interp_run(interp, ex_ret, &ex_argc, &ex_args);
			prog = NULL;
			if (r != APPLY_EXTERNAL_CALL)
				break;
			/* An example for handling external calls: display */

			if (xstring_equal_cstr(ex_args[0]->string, "display", -1))
			{
				for (i = 1; i != ex_argc; ++ i)
				{
					object_dump(ex_args[i]);
				}
				printf("\n");
				
				ex_ret = OBJECT_NULL;
			}
			else ex_ret = OBJECT_NULL;

			/* The caller should unprotect the ex arguments by themself */
			for (i = 0; i != ex_argc; ++ i)
				interp_unprotect(interp, ex_args[i]);
		}

		interp_uninitialize(interp);

		break;
		
	default:

		printf("ERROR OPTION\n");
		return -1;
	}

	return 0;
}
