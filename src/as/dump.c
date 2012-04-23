#include <config.h>
#include "dump.h"

#if SEE_SYSTEM_IO

int
ast_dump(ast_node_t root, SEE_FILE_T out)
{
	int size = 1;
	int max  = 1;
	ast_node_t   *ast_stack;
	int        *level_stack;
	
#define EXPAND_STACK													\
	while (max < size)													\
	{																	\
		max <<= 1;														\
		if ((ast_stack = (ast_node_t *)SEE_REALLOC(ast_stack, sizeof(ast_node_t) * max)) == NULL) \
			return -1;													\
		if ((level_stack = (int *)SEE_REALLOC(level_stack, sizeof(int) * max)) == NULL)	\
		{ SEE_FREE(ast_stack); return -1; }									\
	}

	ast_stack   = (ast_node_t *)SEE_MALLOC(sizeof(ast_node_t));
	level_stack = (int *)SEE_MALLOC(sizeof(int));
	
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
			SEE_FPRINTF(out, "General:");
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
				SEE_FPRINTF(out, "NULL");
				break;
				
			case SYMBOL_GENERAL:
				SEE_FPRINTF(out, "Symbol: %s", xstring_cstr(now->symbol.str));
				break;

			case SYMBOL_NUMERIC:
				SEE_FPRINTF(out, "Numeric: %s", xstring_cstr(now->symbol.str));
				break;

			case SYMBOL_STRING:
				SEE_FPRINTF(out, "String: %s", xstring_cstr(now->symbol.str));
				break;
			}
				
			break;
		}

		case AST_LAMBDA:
		{
			SEE_FPRINTF(out, "Lambda:");
			
			for (i = 0; i != now->lambda.argc; ++ i)
			{
				SEE_FPRINTF(out, " %s", xstring_cstr(now->lambda.args[i]));
			}

			if (now->lambda.tail_list) SEE_FPRINTF(out, "*");

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->lambda.proc;
			level_stack[size - 1] = indent;
			break;
		}

		case AST_WITH:
		{
			SEE_FPRINTF(out, "With:");
			
			for (i = 0; i != now->with.varc; ++ i)
			{
				SEE_FPRINTF(out, " %s", xstring_cstr(now->with.vars[i]));
			}

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->with.proc;
			level_stack[size - 1] = indent;
			break;
		}

		case AST_PROC:
		{
			if (now->proc.toplevel)
				SEE_FPRINTF(out, "TopLevel Proc:");
			else SEE_FPRINTF(out, "Proc:");

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
			SEE_FPRINTF(out, "And:");

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
			SEE_FPRINTF(out, "Or:");

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
			SEE_FPRINTF(out, "Apply: func args");
			
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
			SEE_FPRINTF(out, "If: cond then");
			size += 2;
			EXPAND_STACK;

			if (now->cond.e != NULL)
			{
				SEE_FPRINTF(out, " else");
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
			SEE_FPRINTF(out, "SET: NAME=%s value", xstring_cstr(now->set.name));

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->set.value;
			level_stack[size - 1] = indent + 1;
			break;
		}

		case AST_CALLCC:
		{
			SEE_FPRINTF(out, "CALL/CC:");

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = now->callcc.node;
			level_stack[size - 1] = indent + 1;
			break;
		}

		default:
		{
			SEE_FPRINTF(out, "Unknown AST type");
			break;
		}
		
		}

		SEE_FPRINTF(out, "\n");
		for (i = 0; i < indent; ++ i)
		{
			fputc('-', out);
			fputc('-', out);
		}
		SEE_FPRINTF(out, "----\n");
	}

	SEE_FREE(ast_stack);
	SEE_FREE(level_stack);

	return 0;

#undef EXPAND_STACK
}

#endif
