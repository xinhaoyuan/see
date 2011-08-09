#include <stdlib.h>
#include <stdio.h>

#include "syntax_parse.h"
#include "symbol.h"

#define TOKEN_LAMBDA	"lambda"
#define TOKEN_AND		"and"
#define TOKEN_OR		"or"
#define TOKEN_BEGIN		"begin"
#define TOKEN_WITH		"with"
#define TOKEN_SET		"set!"
#define TOKEN_COND		"if"
#define TOKEN_CALLCC    "call/cc"

ast_node_t
ast_syntax_parse(ast_node_t root, int tail)
{
	if (root->type != AST_GENERAL)
	{
		return root;
	}

	ast_general_t g = (ast_general_t)(root + 1);
	if (g->right == g) // ()
	{
		root = (ast_node_t)realloc(root, sizeof(struct ast_header_s) + sizeof(struct as_symbol_s));
		as_symbol_t s = (as_symbol_t)(root + 1);

		root->type = AST_SYMBOL;
		root->parent = NULL;
		root->next = NULL;

		s->type = SYMBOL_NULL;
		
		return root;
	}

	ast_node_t result = NULL;
	ast_general_t h = g->right;

	int succ = 1;
	as_symbol_t s = NULL;
	if (h->node->type == AST_SYMBOL)
	{
		s = (as_symbol_t)(h->node + 1);
		if (s->type != SYMBOL_GENERAL) s = NULL;
	}

	/* Process LAMBDA */
	if (s != NULL && xstring_equal_cstr(s->str, TOKEN_LAMBDA, -1))
	{
		ast_general_t args_list = h->right;
		ast_general_t body_head = args_list->right;
		int arg_count;
		
		if (args_list == g || body_head == g)
		{
			fprintf(stderr, "BAD SYNTAX FOR LAMBDA\n");
			succ = 0;
		}
		else if (args_list->node->type != AST_GENERAL)
		{
			fprintf(stderr, "BAD ARGS POSITION\n");
			succ = 0;
		}
		else
		{
			ast_general_t a_head = (ast_general_t)(args_list->node + 1);
			ast_general_t a_now  = a_head->right;
			
			arg_count = 0;
			
			while (a_head != a_now)
			{
				++ arg_count;
				
				if (a_now->node->type != AST_SYMBOL ||
					((as_symbol_t)(a_now->node + 1))->type != SYMBOL_GENERAL)
				{
					fprintf(stderr, "ARG MUST BE SYMBOL\n");
					succ = 0;
					break;
				}
				
				a_now = a_now->right;
			}

			if (succ && arg_count > 0)
			{
				a_now = a_head->left;
				if (xstring_equal_cstr(((as_symbol_t)(a_now->node + 1))->str, "...", -1))
				{
					arg_count = -arg_count + 1;
					if (arg_count == 0)
					{
						fprintf(stderr, "INVALID ARG LIST FORMAT\n");
						succ = 0;
					}
				}
			}
		}

		if (succ)
		{
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_lambda_s));
			result->type = AST_LAMBDA;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;

			ast_lambda_t l = (ast_lambda_t)(result + 1);
			if (arg_count > 0)
			{
				l->tail_list = 0;
				l->argc = arg_count;
			}
			else
			{
				l->tail_list = 1;
				l->argc = -arg_count;
			}
			
			l->args = malloc(sizeof(struct xstring_s) * l->argc);

			ast_general_t a_head = (ast_general_t)(args_list->node + 1);
			ast_general_t a_now  = a_head->right;
			int i;
			for (i = 0; i != l->argc; ++ i)
			{
				l->args[i] = ((as_symbol_t)(a_now->node + 1))->str;
				
				a_now = a_now->right;
				free(a_now->left);
			}
			if (a_now != a_head) free(a_now);

			l->proc = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_proc_s));
			l->proc->type = AST_PROC;
			l->proc->parent = result;
			l->proc->next = NULL;
			l->proc->priv = NULL;
			
			ast_proc_t p = (ast_proc_t)(l->proc + 1);
			p->count = 0;
			while (body_head != g)
			{
				body_head->node = ast_syntax_parse(body_head->node, body_head->right == g);
				body_head = body_head->right;
				
				++ p->count;
			}

			body_head = args_list->right;
			p->nodes = (ast_node_t *)malloc(sizeof(ast_node_t) * p->count);
			for (i = 0; i != p->count; ++ i)
			{
				p->nodes[i] = body_head->node;
				p->nodes[i]->parent = l->proc;
				if (i > 0)
					p->nodes[i - 1]->next = p->nodes[i];

				body_head = body_head->right;
				free(body_head->left);
			}

			free(args_list);
			free(h);
			free(root);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_WITH, -1))
	{
		ast_general_t vars_list = h->right;
		ast_general_t body_head = vars_list->right;
		int var_count;
		
		if (vars_list == g || body_head == g)
		{
			fprintf(stderr, "BAD SYNTAX FOR WITH\n");
			succ = 0;
		}
		else if (vars_list->node->type != AST_GENERAL)
		{
			fprintf(stderr, "BAD VARS POSITION\n");
			succ = 0;
		}
		else
		{
			ast_general_t v_head = (ast_general_t)(vars_list->node + 1);
			ast_general_t v_now  = v_head->right;
			
			if (v_head == v_now)
			{
				fprintf(stderr, "BAD VARS LIST\n");
				succ = 0;
			}
			else
			{
				var_count = 0;
				
				while (v_head != v_now)
				{
					++ var_count;
					
					if (v_now->node->type != AST_SYMBOL ||
						((as_symbol_t)(v_now->node + 1))->type != SYMBOL_GENERAL)
					{
						fprintf(stderr, "VAR MUST BE SYMBOL\n");
						succ = 0;
						break;
					}
					
					v_now = v_now->right;
				}
			}
		}

		if (succ)
		{
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_with_s));
			result->type = AST_WITH;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;

			ast_with_t w = (ast_with_t)(result + 1);
			w->varc = var_count;
			w->vars = malloc(sizeof(struct xstring_s) * w->varc);

			ast_general_t v_head = (ast_general_t)(vars_list->node + 1);
			ast_general_t v_now  = v_head->right;
			int i;
			for (i = 0; i != w->varc; ++ i)
			{
				w->vars[i] = ((as_symbol_t)(v_now->node + 1))->str;
				
				v_now = v_now->right;
				free(v_now->left);
			}

			w->proc = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_proc_s));
			w->proc->type = AST_PROC;
			w->proc->parent = result;
			w->proc->next = NULL;
			w->proc->priv = NULL;
			
			ast_proc_t p = (ast_proc_t)(w->proc + 1);
			p->count = 0;
			while (body_head != g)
			{
				body_head->node = ast_syntax_parse(body_head->node, tail && body_head->right == g);
				body_head = body_head->right;
				
				++ p->count;
			}

			body_head = vars_list->right;
			p->nodes = (ast_node_t *)malloc(sizeof(ast_node_t) * p->count);
			for (i = 0; i != p->count; ++ i)
			{
				p->nodes[i] = body_head->node;
				p->nodes[i]->parent = w->proc;
				if (i > 0)
					p->nodes[i - 1]->next =	p->nodes[i];

				body_head = body_head->right;
				free(body_head->left);
			}

			free(vars_list);
			free(h);
			free(root);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_COND, -1))
	{
		ast_general_t c = h->right;
		ast_general_t t = c->right;
		ast_general_t e = t->right;

		if (c == g || t == g || (e != g && e->right != g))
		{
			fprintf(stderr, "Invalid format for IF\n");
			succ = 0;
		}

		if (succ)
		{
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_cond_s));
			result->type = AST_COND;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;
			
			ast_cond_t i = (ast_cond_t)(result + 1);

			i->c = ast_syntax_parse(c->node, 0);
			i->t = ast_syntax_parse(t->node, tail);
			free(c);
			free(t);

			i->c->parent = result;
			i->c->next   = NULL;
			i->t->parent = result;
			i->t->next   = NULL;
						

			if (e == g)
				i->e = NULL;
			else
			{
				i->e = ast_syntax_parse(e->node, tail);
				free(e);

				i->e->parent = result;
				i->e->next   = NULL;
			}
			
			free(h);
			free(root);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_SET, -1))
	{
		ast_general_t n = h->right;
		ast_general_t v = n->right;

		if (n == g || v == g || v->right != g)
		{
			
			succ = 0;
			/* TODO ERROR */
		}
		else if (n->node->type != AST_SYMBOL ||
				 ((as_symbol_t)n->node + 1)->type != AST_GENERAL)
		{
			succ = 0;
			/* TODO ERROR */
		}

		if (succ)
		{
			result = malloc(sizeof(struct ast_header_s) + sizeof(struct ast_set_s));
			result->type = AST_SET;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;
			
			ast_set_t s = (ast_set_t)(result + 1);
			
			s->name = ((as_symbol_t)(n->node + 1))->str;
			s->value = ast_syntax_parse(v->node, 0);
			s->value->parent = result;
			s->value->next = NULL;
			s->value->priv = NULL;

			free(n);
			free(v);
			free(h);
			free(root);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_BEGIN, -1))
	{
		int s_count = 0;
		ast_general_t s_now = h->right;

		if (s_now == g)
		{
			fprintf(stderr, "not empty for begin\n");
			succ = 0;
		}

		if (succ)
		{
			
			while (s_now != g)
			{
				++ s_count;
				s_now = s_now->right;
			}
			
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_proc_s));
			ast_proc_t p = (ast_proc_t)(result + 1);
			
			result->type = AST_PROC;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;
			
			p->count = s_count;
			p->nodes = (ast_node_t *)malloc(sizeof(ast_node_t) * p->count);
			
			s_now = h->right;
			int i;
			for (i = 0; i != p->count; ++ i)
			{
				p->nodes[i] = ast_syntax_parse(s_now->node, tail && s_now->right == g);
				p->nodes[i]->parent = result;
				if (i > 0)
					p->nodes[i - 1]->next = p->nodes[i];
				
				s_now = s_now->right;
				free(s_now->left);
			}
			
			free(h);
			free(root);
		}

	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_AND, -1))
	{
		int s_count = 0;
		ast_general_t s_now = h->right;

		if (s_now == g)
		{
			fprintf(stderr, "not empty for and\n");
			succ = 0;
		}

		if (succ)
		{

			while (s_now != g)
			{
				++ s_count;
				s_now = s_now->right;
			}
			
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_and_s));
			ast_and_t p = (ast_and_t)(result + 1);
			
			result->type = AST_AND;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;
			
			p->count = s_count;
			p->nodes = (ast_node_t *)malloc(sizeof(ast_node_t) * p->count);
			
			s_now = h->right;
			int i;
			for (i = 0; i != p->count; ++ i)
			{
				p->nodes[i] = ast_syntax_parse(s_now->node, tail && s_now->right == g);
				p->nodes[i]->parent = result;
				if (i > 0)
					p->nodes[i - 1]->next = p->nodes[i];
				
				s_now = s_now->right;
				free(s_now->left);
			}
			
			free(h);
			free(root);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_OR, -1))
	{
		int s_count = 0;
		ast_general_t s_now = h->right;

		if (s_now == g)
		{
			fprintf(stderr, "not empty for or\n");
			succ = 0;
		}

		if (succ)
		{
			
			while (s_now != g)
			{
				++ s_count;
				s_now = s_now->right;
			}
			
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_or_s));
			ast_or_t p = (ast_or_t)(result + 1);
			
			result->type = AST_OR;
			result->parent = NULL;
			result->next = NULL;
			result->priv = root->priv;
			
			p->count = s_count;
			p->nodes = (ast_node_t *)malloc(sizeof(ast_node_t) * p->count);
			
			s_now = h->right;
			int i;
			for (i = 0; i != s_count; ++ i)
			{
				p->nodes[i] = ast_syntax_parse(s_now->node, tail && s_now->right == g);
				p->nodes[i]->parent = result;
				if (i > 0)
					p->nodes[i - 1]->next = p->nodes[i];
				
				s_now = s_now->right;
				free(s_now->left);
			}
			
			free(h);
			free(root);
		}
	}
	else if (s != NULL && xstring_equal_cstr(s->str, TOKEN_CALLCC, -1))
	{
		ast_general_t n = h->right;
		if (n == g || n->right != g)
		{
			fprintf(stderr, "ERROR FORMAT FOR CALL/CC\n");
			succ = 0;
		}

		if (succ)
		{
			result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_callcc_s));
			ast_callcc_t c = (ast_callcc_t)(result + 1);

			result->type = AST_CALLCC;
			result->parent = NULL;
			result->next = NULL;
			result->priv = NULL;

			c->node = ast_syntax_parse(n->node, 0);
			c->node->parent = result;
			c->node->next = NULL;
			c->tail = tail;

			free(n);
			free(h);
			free(root);
		}
	}
	else s = NULL;

	/* PROCESS APPLY */
	if (s == NULL)
	{
		int a_count = 0;
		ast_general_t a_now = h->right;

		while (a_now != g)
		{
			++ a_count;
			a_now = a_now->right;
		}

		result = (ast_node_t)malloc(sizeof(struct ast_header_s) + sizeof(struct ast_apply_s));
		ast_apply_t a = (ast_apply_t)(result + 1);

		result->type = AST_APPLY;
		result->parent = NULL;
		result->next = NULL;
		result->priv = root->priv;

		a->argc = a_count;
		a->tail = tail;
		a->func = ast_syntax_parse(h->node, 0);
		a->func->parent = result;

		a->args = (ast_node_t *)malloc(sizeof(ast_node_t) * a->argc);
		
		a_now = h->right;
		int i;
		for (i = 0; i != a_count; ++ i)
		{
			a->args[i] = ast_syntax_parse(a_now->node, 0);
			a->args[i]->parent = result;

			if (i > 0)
				a->args[i - 1]->next = a->args[i];
			else a->func->next = a->args[0];

			a_now = a_now->right;
			free(a_now->left);
		}

		free(h);
		free(root);
	}

	if (succ)
		return result;
	else return root;
}
