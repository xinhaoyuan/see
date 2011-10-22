#ifndef __SEE_AS_AST_H__
#define __SEE_AS_AST_H__

#include "../lib/xstring.h"

#define AST_GENERAL   0
#define AST_SYMBOL    1
#define AST_APPLY     2
#define AST_SET       3
#define AST_COND      4
#define AST_LAMBDA    5
#define AST_WITH      6
#define AST_PROC      7
#define AST_AND       8
#define AST_OR        9
#define AST_CALLCC    10

#define SYMBOL_NULL    0
#define SYMBOL_GENERAL 1
#define SYMBOL_NUMERIC 2
#define SYMBOL_STRING  3

typedef struct ast_node_s *ast_node_t;
typedef struct ast_node_s
{
	struct ast_header_s
	{
		int        type;
		ast_node_t prev;
		ast_node_t next;
		void      *priv;
	} header;

	union
	{
		struct
		{
			int       type;
			xstring_t str;
		} symbol;
		
		struct 
		{
			ast_node_t head;
		} general;

		struct 
		{
			int         tail;
			ast_node_t  func;
			int         argc;
			ast_node_t *args;
		} apply;

		struct 
		{
			xstring_t     name;
			ast_node_t    value;
		} set;

		struct
		{
			ast_node_t c;
			ast_node_t t;
			ast_node_t e;
		} cond;

		struct
		{
			int         tail_list;
			int         argc;
			xstring_t  *args;
			ast_node_t  proc;
		} lambda;

		struct
		{
			int         varc;
			xstring_t  *vars;
			ast_node_t  proc;
		} with;

		struct
		{
			ast_node_t head;
		} proc;

		struct
		{
			ast_node_t head;
		} s_and;

		struct
		{
			ast_node_t head;
		} s_or;

		struct
		{
			int tail;
			ast_node_t node;
		} callcc;

	};
} ast_node_s;

#endif
