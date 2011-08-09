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
		ast_stack = (ast_node_t *)realloc(ast_stack, sizeof(ast_node_t) * max);	\
		level_stack = (int *)realloc(level_stack, sizeof(int) * max);	\
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
		
		switch (now->type)
		{

		case AST_GENERAL:
		{
			fprintf(out, "General:");
			
			ast_general_t g = (ast_general_t)(now + 1);
			ast_general_t n = g->left;

			while (n != g)
			{
				size += 1;
				EXPAND_STACK;
				
				ast_stack[size - 1]   = n->node;
				level_stack[size - 1] = indent + 1;

				n = n->left;
			}

			break;
		}
		
		case AST_SYMBOL:
		{
			int type = ((as_symbol_t)(now + 1))->type;
			switch (type)
			{
			case SYMBOL_NULL:
				fprintf(out, "NULL");
				break;
				
			case SYMBOL_GENERAL:
				fprintf(out, "Symbol: %s", xstring_cstr(((as_symbol_t)(now + 1))->str));
				break;

			case SYMBOL_NUMERIC:
				fprintf(out, "Numeric: %s", xstring_cstr(((as_symbol_t)(now + 1))->str));
				break;

			case SYMBOL_STRING:
				fprintf(out, "String: %s", xstring_cstr(((as_symbol_t)(now + 1))->str));
				break;
			}
				
			break;
		}

		case AST_LAMBDA:
		{
			ast_lambda_t l = (ast_lambda_t)(now + 1);
			fprintf(out, "Lambda:");
			
			for (i = 0; i != l->argc; ++ i)
			{
				fprintf(out, " %s", xstring_cstr(l->args[i]));
			}

			if (l->tail_list) fprintf(out, "*");

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = l->proc;
			level_stack[size - 1] = indent;
			break;
		}

		case AST_WITH:
		{
			ast_with_t w = (ast_with_t)(now + 1);
			fprintf(out, "With:");
			
			for (i = 0; i != w->varc; ++ i)
			{
				fprintf(out, " %s", xstring_cstr(w->vars[i]));
			}

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = w->proc;
			level_stack[size - 1] = indent;
			break;
		}


		case AST_PROC:
		{
			ast_proc_t p = (ast_proc_t)(now + 1);
			fprintf(out, "Proc:");

			size += p->count;
			EXPAND_STACK;
			
			for (i = 0; i < p->count; ++ i)
			{
				ast_stack[size - i - 1] = p->nodes[i];
				level_stack[size - i - 1] = indent + 1;
			}

			break;
		}

		case AST_AND:
		{
			ast_and_t p = (ast_and_t)(now + 1);
			fprintf(out, "And:");

			size += p->count;
			EXPAND_STACK;
			
			for (i = 0; i < p->count; ++ i)
			{
				ast_stack[size - i - 1] = p->nodes[i];
				level_stack[size - i - 1] = indent + 1;
			}

			break;
		}

		case AST_OR:
		{
			ast_or_t p = (ast_or_t)(now + 1);
			fprintf(out, "Or:");

			size += p->count;
			EXPAND_STACK;
			
			for (i = 0; i < p->count; ++ i)
			{
				ast_stack[size - i - 1] = p->nodes[i];
				level_stack[size - i - 1] = indent + 1;
			}

			break;
		}


		case AST_APPLY:
		{
			ast_apply_t apply = (ast_apply_t)(now + 1);
			fprintf(out, "Apply: func args");
			
			size += apply->argc + 1;
			EXPAND_STACK;
			
			ast_stack[size - 1] = apply->func;
			level_stack[size - 1] = indent + 1;
			for (i = 0; i < apply->argc; ++ i)
			{
				ast_stack[size - i - 2] = apply->args[i];
				level_stack[size - i - 2] = indent + 1;
			}

			break;
		}

		case AST_COND:
		{
			ast_cond_t cond = (ast_cond_t)(now + 1);
			fprintf(out, "If: cond then");
			size += 2;
			EXPAND_STACK;

			if (cond->e != NULL)
			{
				fprintf(out, " else");
				size += 1;
				EXPAND_STACK;
			}

			ast_stack[size - 1] = cond->c;
			ast_stack[size - 2] = cond->t;
			
			level_stack[size - 1] = indent + 1;
			level_stack[size - 2] = indent + 1;

			if (cond->e != NULL)
			{
				ast_stack[size - 3] = cond->e;
				level_stack[size - 3] = indent + 1;
			}

			break;
		}
		
		case AST_SET:
		{
			ast_set_t s = (ast_set_t)(now + 1);
			fprintf(out, "SET: NAME=%s value", xstring_cstr(s->name));

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = s->value;
			level_stack[size - 1] = indent + 1;
			break;
		}

		case AST_CALLCC:
		{
			ast_callcc_t s = (ast_callcc_t)(now + 1);
			fprintf(out, "CALL/CC:");

			size += 1;
			EXPAND_STACK;

			ast_stack[size - 1] = s->node;
			level_stack[size - 1] = indent + 1;
			break;
		}

		default:
		{
			fprintf(out, "Unknown AST type");
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
