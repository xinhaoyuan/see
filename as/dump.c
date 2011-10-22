#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dump.h"

int
ast_dump(ast_node_t root, FILE *out)
{
	int size = 1;
	int max  = 1;
	ast_node_t   *ast_stack;
	int        *level_stack;
	
#define EXPAND_STACK													\
	while (max < size)													\
	{																	\
		max <<= 1;														\
		if ((ast_stack = (ast_node_t *)realloc(ast_stack, sizeof(ast_node_t) * max)) == NULL) \
			return -1;													\
		if ((level_stack = (int *)realloc(level_stack, sizeof(int) * max)) == NULL)	\
		{ free(ast_stack); return -1; }									\
	}

	ast_stack   = (ast_node_t *)malloc(sizeof(ast_node_t));
	level_stack = (int *)malloc(sizeof(int));
	
	ast_stack[size - 1] = root;
	level_stack[size - 1] = 0;

	while (size > 0)
	{
		-- size;
		
		ast_node_t now = ast_stack[size];
		int indent = level_stack[size];

		int i;
		for (i = 0; i < indent; ++ i)
		{
			fputc('.', out);
			fputc('.', out);
		}
		
		switch (now->header.type)
		{

		case AST_GENERAL:
		{
			fprintf(out, "General:");
			ast_node_t c = now->general.head->header.prev;

			while (1)
			{
				size += 1;
				EXPAND_STACK;
				
				ast_stack[size - 1]   = c;
				level_stack[size - 1] = indent + 1;

				if (c == now->general.head) break;
				c = c->header.prev;
			}

			break;
		}
		
		case AST_SYMBOL:
		{
			int type = now->symbol.type;
			switch (type)
			{
			case SYMBOL_NULL:
				fprintf(out, "NULL");
				break;
				
			case SYMBOL_GENERAL:
				fprintf(out, "Symbol: %s", xstring_cstr(now->symbol.str));
				break;

			case SYMBOL_NUMERIC:
				fprintf(out, "Numeric: %s", xstring_cstr(now->symbol.str));
				break;

			case SYMBOL_STRING:
				fprintf(out, "String: %s", xstring_cstr(now->symbol.str));
				break;
			}
				
			break;
		}

		case AST_LAMBDA:
		{
			fprintf(out, "Lambda:");
			
			for (i = 0; i != now->lambda.argc; ++ i)
			{
				fprintf(out, " %s", xstring_cstr(now->lambda.args[i]));
			}

			if (now->lambda.tail_list) fprintf(out, "*");

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->lambda.proc;
			level_stack[size - 1] = indent;
			break;
		}

		case AST_WITH:
		{
			fprintf(out, "With:");
			
			for (i = 0; i != now->with.varc; ++ i)
			{
				fprintf(out, " %s", xstring_cstr(now->with.vars[i]));
			}

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->with.proc;
			level_stack[size - 1] = indent;
			break;
		}


		case AST_PROC:
		{
			fprintf(out, "Proc:");

			ast_node_t cur = now->proc.head->header.prev;
			while (cur != NULL)
			{
				
				size += 1;
				EXPAND_STACK;
				ast_stack[size - 1] = cur;
				level_stack[size - 1] = indent + 1;

				if (cur == now->proc.head) cur = NULL;
				else cur = cur->header.prev;
			}
			break;
		}

		case AST_AND:
		{
			fprintf(out, "And:");

			ast_node_t cur = now->s_and.head->header.prev;
			while (cur != NULL)
			{
				
				size += 1;
				EXPAND_STACK;
				ast_stack[size - 1] = cur;
				level_stack[size - 1] = indent + 1;

				if (cur == now->s_and.head) cur = NULL;
				else cur = cur->header.prev;
			}
			
			break;
		}

		case AST_OR:
		{
			fprintf(out, "Or:");

			ast_node_t cur = now->s_or.head->header.prev;
			while (cur != NULL)
			{
				
				size += 1;
				EXPAND_STACK;
				ast_stack[size - 1] = cur;
				level_stack[size - 1] = indent + 1;

				if (cur == now->s_or.head) cur = NULL;
				else cur = cur->header.prev;
			}

			break;
		}


		case AST_APPLY:
		{
			fprintf(out, "Apply: func args");
			
			size += now->apply.argc + 1;
			EXPAND_STACK;
			
			ast_stack[size - 1] = now->apply.func;
			level_stack[size - 1] = indent + 1;
			for (i = 0; i < now->apply.argc; ++ i)
			{
				ast_stack[size - i - 2] = now->apply.args[i];
				level_stack[size - i - 2] = indent + 1;
			}

			break;
		}

		case AST_COND:
		{
			fprintf(out, "If: cond then");
			size += 2;
			EXPAND_STACK;

			if (now->cond.e != NULL)
			{
				fprintf(out, " else");
				size += 1;
				EXPAND_STACK;
			}

			ast_stack[size - 1] = now->cond.c;
			ast_stack[size - 2] = now->cond.t;
			
			level_stack[size - 1] = indent + 1;
			level_stack[size - 2] = indent + 1;

			if (now->cond.e != NULL)
			{
				ast_stack[size - 3] = now->cond.e;
				level_stack[size - 3] = indent + 1;
			}

			break;
		}
		
		case AST_SET:
		{
			fprintf(out, "SET: NAME=%s value", xstring_cstr(now->set.name));

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->set.value;
			level_stack[size - 1] = indent + 1;
			break;
		}

		case AST_CALLCC:
		{
			fprintf(out, "CALL/CC:");

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->callcc.node;
			level_stack[size - 1] = indent + 1;
			break;
		}

		default:
		{
			fprintf(out, "Unknown AST type");
			break;
		}
		
		}

		fprintf(out, "\n");
		for (i = 0; i < indent; ++ i)
		{
			fputc('-', out);
			fputc('-', out);
		}
		fprintf(out, "----\n");
	}

	free(ast_stack);
	free(level_stack);

	return 0;

#undef EXPAND_STACK
};
