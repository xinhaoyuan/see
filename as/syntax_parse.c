#include <stdlib.h>
#include <stdio.h>

#include "syntax_parse.h"

#define TOKEN_LAMBDA	"lambda"
#define TOKEN_AND		"and"
#define TOKEN_OR		"or"
#define TOKEN_BEGIN		"begin"
#define TOKEN_WITH		"with"
#define TOKEN_SET		"set!"
#define TOKEN_COND		"if"
#define TOKEN_CALLCC    "call/cc"

/* Parse the ast tree */
/* Return 0 means success */
int
ast_syntax_parse(ast_node_t root, int tail)
{
	/* parse only unparsed list */
	if (root->header.type != AST_GENERAL)
		return 0;

	if (root->general.head == NULL) // ()
	{
		root->header.type = AST_SYMBOL;
		root->symbol.type = SYMBOL_NULL;
		
		return 0;
	}

	ast_node_t h = root->general.head;

	int succ = 1;
	/* The head symbol */
	xstring_t s = NULL;
	if (h->header.type == AST_SYMBOL && h->symbol.type == SYMBOL_GENERAL)
		s = h->symbol.str;

#define PROCESS_LIST(HEAD, TAIL, TAILFLAG)								\
	do																	\
	{																	\
		ast_node_t __cur = (HEAD);										\
		int __tail = 0;													\
		while (!__tail)													\
		{																\
			__tail = __cur->header.next == (TAIL);						\
			if (ast_syntax_parse(__cur,									\
								 (TAILFLAG) && __tail) != 0) succ = 0;	\
			__cur = __cur->header.next;									\
		}																\
	} while (0)

	/* Process LAMBDA */
	if (s != NULL && xstring_equal_cstr(s, TOKEN_LAMBDA, -1))
	{
		ast_node_t args_list = h->header.next;
		ast_node_t body_head = args_list->header.next;
		int args_count;
		
		if (args_list == h || body_head == h)
		{
			fprintf(stderr, "BAD SYNTAX FOR LAMBDA\n");
			succ = 0;
		}
		else if (args_list->header.type != AST_GENERAL)
		{
			fprintf(stderr, "BAD ARGS POSITION\n");
			succ = 0;
		}
		else
		{
			ast_node_t a_now = args_list->general.head;
			args_count = 0;
			
			while (a_now != NULL)
			{
				++ args_count;
				
				if (a_now->header.type != AST_SYMBOL ||
					a_now->symbol.type != SYMBOL_GENERAL)
				{
					fprintf(stderr, "ARG MUST BE SYMBOL\n");
					succ = 0;
					break;
				}
				
				a_now = a_now->header.next;
				if (a_now == args_list->general.head) break;
			}

			if (succ && args_count > 0)
			{
				a_now = args_list->general.head->header.prev;
				if (xstring_equal_cstr(a_now->symbol.str, "...", -1))
				{
					args_count = -args_count + 1;
					if (args_count == 0)
					{
						fprintf(stderr, "INVALID ARG LIST FORMAT\n");
						succ = 0;
					}
				}
			}
		}

		if (succ)
			PROCESS_LIST(body_head, h, 1);

		ast_node_t proc;
		xstring_t *args;
		
		if (succ)
			succ = !!(proc = (ast_node_t)malloc(sizeof(struct ast_node_s)));

		if (succ)
		{
			if (!(succ = !!(args = (xstring_t *)malloc(
								sizeof(xstring_t) *
								(args_count > 0 ? args_count : -args_count)))))
				free(proc);
		}

		if (succ)
		{
			root->header.type = AST_LAMBDA;

			if (args_count > 0)
			{
				root->lambda.tail_list = 0;
				root->lambda.argc = args_count;
			}
			else
			{
				root->lambda.tail_list = 1;
				root->lambda.argc = -args_count;
			}
			
			root->lambda.args = args;

			ast_node_t a_now = args_list->general.head;
			int i;
			for (i = 0; i != root->lambda.argc; ++ i)
			{
				root->lambda.args[i] = a_now->symbol.str;
				ast_node_t last = a_now;
				a_now = a_now->header.next;
				free(last);
			}
			if (a_now != args_list->general.head)
			{
				// Free the "..."
				xstring_free(a_now->symbol.str);
				free(a_now);
			}

			root->lambda.proc = proc;
			proc->header.type = AST_PROC;
			proc->header.prev = proc->header.next = proc;
			proc->header.priv = NULL;
			proc->proc.head = body_head;
			
			body_head->header.prev = h->header.prev;
			body_head->header.prev->header.next = body_head;

			free(args_list);
			free(h);
			xstring_free(s);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_WITH, -1))
	{
		ast_node_t vars_list = h->header.next;
		ast_node_t body_head = vars_list->header.next;
		int vars_count;
		
		if (vars_list == h || body_head == h)
		{
			fprintf(stderr, "BAD SYNTAX FOR WITH\n");
			succ = 0;
		}
		else if (vars_list->header.type != AST_GENERAL)
		{
			fprintf(stderr, "BAD VARS POSITION\n");
			succ = 0;
		}
		else
		{
			ast_node_t v_now  = vars_list->general.head;
			
			if (v_now == NULL)
			{
				fprintf(stderr, "BAD VARS LIST\n");
				succ = 0;
			}
			else
			{
				vars_count = 0;
				
				while (v_now != NULL)
				{
					++ vars_count;
					
					if (v_now->header.type != AST_SYMBOL ||
						v_now->symbol.type != SYMBOL_GENERAL)
					{
						fprintf(stderr, "VAR MUST BE SYMBOL\n");
						succ = 0;
						break;
					}
					
					v_now = v_now->header.next;
					if (v_now == vars_list->general.head) v_now = NULL;
				}
			}
		}

		if (succ)
			PROCESS_LIST(body_head, h, tail);

		xstring_t *vars;
		ast_node_t proc;

		if (succ)
			succ = !!(proc = (ast_node_t)malloc(sizeof(struct ast_node_s)));

		if (succ)
		{
			if (!(succ = !!(vars = (xstring_t *)malloc(
								sizeof(xstring_t) *
								vars_count))))
				free(proc);
		}

		if (succ)
		{
			root->header.type = AST_WITH;

			root->with.varc = vars_count;
			root->with.vars = vars;

			ast_node_t v_now  = vars_list->general.head;
			int i;
			for (i = 0; i != root->with.varc; ++ i)
			{
				root->with.vars[i] = v_now->symbol.str;
				ast_node_t last = v_now;
				v_now = v_now->header.next;
				free(last);
			}
			
			root->with.proc = proc;
			proc->header.type = AST_PROC;
			proc->header.prev = proc->header.next = proc;
			proc->header.priv = NULL;
			proc->proc.head = body_head;

			body_head->header.prev = h->header.prev;
			body_head->header.prev->header.next = body_head;

			free(vars_list);
			free(h);
			xstring_free(s);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_COND, -1))
	{
		ast_node_t c = h->header.next;
		ast_node_t t = c->header.next;
		ast_node_t e = t->header.next;

		if (c == h || t == h || (e != h && e->header.next != h))
		{
			fprintf(stderr, "Invalid format for IF\n");
			succ = 0;
		}

		if (succ)
			succ = !ast_syntax_parse(c, 0);

		if (succ)
			succ = !ast_syntax_parse(t, tail);

		if (succ && e != h)
			succ = !ast_syntax_parse(e, tail);

		if (succ)
		{
			root->header.type = AST_COND;
			
			c->header.prev = c->header.next = c;
			t->header.prev = t->header.next = t;

			root->cond.c = c;
			root->cond.t = t;

			if (e == h)
				root->cond.e = NULL;
			else
			{
				e->header.prev = e->header.next = e;
				root->cond.e = e;
			}
			
			free(h);
			xstring_free(s);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_SET, -1))
	{
		ast_node_t n = h->header.next;
		ast_node_t v = n->header.next;

		if (n == h || v == h || v->header.next != h)
		{
			
			succ = 0;
			/* TODO ERROR */
		}
		else if (n->header.type != AST_SYMBOL ||
				 n->symbol.type != SYMBOL_GENERAL)
		{
			succ = 0;
			/* TODO ERROR */
		}

		if (succ)
			succ = !ast_syntax_parse(v, 0);
		
		if (succ)
		{
			root->header.type = AST_SET;
			
			root->set.name = n->symbol.str;
			v->header.next = v->header.prev = v;
			root->set.value = v;

			free(n);
			free(h);
			xstring_free(s);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_BEGIN, -1))
	{
		ast_node_t s_now = h->header.next;

		if (s_now == h)
		{
			fprintf(stderr, "not empty for begin\n");
			succ = 0;
		}

		if (succ)
			PROCESS_LIST(s_now, h, tail);
		
		if (succ)
		{
			root->header.type = AST_PROC;
			root->proc.head = s_now;

			s_now->header.prev = h->header.prev;
			s_now->header.prev->header.next = s_now;
			
			free(h);
			xstring_free(s);
		}

	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_AND, -1))
	{
		ast_node_t s_now = h->header.next;

		if (s_now == h)
		{
			fprintf(stderr, "not empty for and\n");
			succ = 0;
		}

		if (succ)
			PROCESS_LIST(s_now, h, tail);

		if (succ)
		{
			root->header.type = AST_AND;
			root->s_and.head = s_now;

			s_now->header.prev = h->header.prev;
			s_now->header.prev->header.next = s_now;
			
			free(h);
			xstring_free(s);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_OR, -1))
	{
		ast_node_t s_now = h->header.next;

		if (s_now == h)
		{
			fprintf(stderr, "not empty for and\n");
			succ = 0;
		}

		if (succ)
			PROCESS_LIST(s_now, h, tail);
		
		if (succ)
		{
			root->header.type = AST_AND;
			root->s_and.head = s_now;

			s_now->header.prev = h->header.prev;
			s_now->header.prev->header.next = s_now;
			
			free(h);
			xstring_free(s);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s, TOKEN_CALLCC, -1))
	{
		ast_node_t n = h->header.next;
		if (n == h || n->header.next != h)
		{
			fprintf(stderr, "ERROR FORMAT FOR CALL/CC\n");
			succ = 0;
		}

		if (succ)
			succ = !ast_syntax_parse(n, 0);
		
		if (succ)
		{
			root->header.type = AST_CALLCC;
			n->header.next = n->header.prev = n;
			root->callcc.node = n;
			root->callcc.tail = tail;

			free(h);
			xstring_free(s);
		}
	}
	else
	{
		/* PROCESS APPLY */
		int a_count = 0;
		ast_node_t a_now = h->header.next;
		
		succ = !ast_syntax_parse(h, 0);
		
		if (succ)
		{
			while (a_now != h)
			{
				++ a_count;
				if (ast_syntax_parse(a_now, 0)) succ = 0;
				a_now = a_now->header.next;
			}
		}
		
		ast_node_t *args = NULL;
		if (succ && a_count > 0)
			succ = !!(args = (ast_node_t *)malloc(sizeof(ast_node_t) * a_count));
		
		if (succ)
		{
			root->header.type = AST_APPLY;
		
			root->apply.argc = a_count;
			root->apply.tail = tail;
			root->apply.func = h;
			
			root->apply.args = args;
			
			a_now = h->header.next;
			int i;
			for (i = 0; i != a_count; ++ i)
			{
				root->apply.args[i] = a_now;
				a_now = a_now->header.next;
			}
		}
	}

	return !succ;
}
