#ifndef __SEE_AS_AST_H__
#define __SEE_AS_AST_H__

#include "symbol.h"

#include <xstring.h>

#define AST_GENERAL   0
#define AST_SYMBOL    1
#define AST_APPLY     2
#define AST_SET       3
#define AST_COND        4
#define AST_LAMBDA    5
#define AST_WITH      6
#define AST_PROC      7
#define AST_AND       8
#define AST_OR        9
#define AST_CALLCC    10

struct ast_header_s
{
	int                  type;
	struct ast_header_s *parent;
	struct ast_header_s *next;
	void                *priv;
};

typedef struct ast_header_s *ast_node_t;

typedef struct ast_general_s
{
	ast_node_t node;
	struct ast_general_s *left, *right;
} *ast_general_t;

typedef struct ast_apply_s
{
	int         tail;
	ast_node_t  func;
	int         argc;
	ast_node_t *args;
} *ast_apply_t;

typedef struct ast_set_s
{
	xstring_t     name;
	ast_node_t    value;
} *ast_set_t;

typedef struct ast_cond_s
{
	ast_node_t c;
	ast_node_t t;
	ast_node_t e;
} *ast_cond_t;

typedef struct ast_lambda_s
{
	int         tail_list;
	int         argc;
	xstring_t  *args;
	ast_node_t  proc;
} *ast_lambda_t;

typedef struct ast_with_s
{
	int         varc;
	xstring_t  *vars;
	ast_node_t  proc;
} *ast_with_t;

typedef struct ast_proc_s
{
	int         count;
	ast_node_t *nodes;
} *ast_proc_t;

typedef struct ast_and_s
{
	int         count;
	ast_node_t *nodes;
} *ast_and_t;

typedef struct ast_or_s
{
	int         count;
	ast_node_t *nodes;
} *ast_or_t;

typedef struct ast_callcc_s
{
	int tail;
	ast_node_t node;
} *ast_callcc_t;

#endif
