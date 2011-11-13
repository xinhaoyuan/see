#include <stdlib.h>
#include <stdio.h>

#include "simple_parse.h"
#include "free.h"

#include "../lib/xstring.h"

#define TOKEN_EOF    0
#define TOKEN_LC     1
#define TOKEN_RC     2
#define TOKEN_SYMBOL 3
#define TOKEN_STRING 4

#define CHAR_IS_SEPARATOR(c) ((c) == '\n' || (c) == '\r' || (c) == ' ' || (c) == '\t')
#define CHAR_IS_NUMERIC(c) ((c) <= '9' && (c) >= '0')
#define CHAR_IS_NEWLINE(c) ((c) == '\n' || (c) == '\r')

#define TOKEN_CHAR_NUMERIC_CONSTANT '#'
#define TOKEN_CHAR_MINUS   '-'
#define TOKEN_CHAR_DOT     '.'
#define TOKEN_CHAR_LC      '('
#define TOKEN_CHAR_RC      ')'
#define TOKEN_CHAR_QUOTE   '"'
#define TOKEN_CHAR_ESCAPE  '\\'
#define TOKEN_CHAR_COMMENT ';'

/* Parse a token from the input stream */
/* return the type of token, -1 mean some error */
static int
parse_token(stream_in_f stream_in, void *stream, xstring_t *result)
{
	int   type;
	char *token_buf;
	int   token_len;
	int   token_buf_alloc = 0;

	type = -1;
	*result = NULL;
	token_buf = NULL;
	token_len = 0;

#define EXPAND_TOKEN													\
	while (token_buf_alloc < token_len)									\
	{																	\
		token_buf_alloc = (token_buf_alloc << 1) + 1;					\
		if (token_buf == NULL)											\
			token_buf = (char *)malloc(sizeof(char) * token_buf_alloc); \
		else token_buf = (char *)realloc(token_buf, sizeof(char) * token_buf_alloc); \
		if (token_buf == NULL) return -1;								\
	}
	
	while (1)
	{
		int now = stream_in(stream, 0);
		if (now < 0)
		{
			if (token_len > 0)
			{
				type = TOKEN_SYMBOL;
				*result = xstring_from_cstr(token_buf, token_len);
			}
			else type = TOKEN_EOF;
			break;
		}

		if (CHAR_IS_SEPARATOR(now))
		{
			while (1)
			{
				stream_in(stream, 1);
				now = stream_in(stream, 0);
				if (CHAR_IS_SEPARATOR(now)) continue;
				break;
			}
			
			if (token_len > 0)
			{
				type = TOKEN_SYMBOL;
				*result = xstring_from_cstr(token_buf, token_len);
				break;
			}
			else if (now < 0) break;
		}

		if (now == TOKEN_CHAR_LC || now == TOKEN_CHAR_RC)
		{
			if (token_len > 0)
			{
				type = TOKEN_SYMBOL;
				*result = xstring_from_cstr(token_buf, token_len);
				break;
			}
			else
			{
				stream_in(stream, 1);
				
				if (now == TOKEN_CHAR_LC)
					type = TOKEN_LC;
				else type = TOKEN_RC;
				
				break;
			}
		}

		if (now == TOKEN_CHAR_COMMENT)
		{
			stream_in(stream, 1);
			while (1)
			{
				now = stream_in(stream, 0);
				if (now < 0 || CHAR_IS_NEWLINE(now))
				{
					while (1)
					{
						now = stream_in(stream, 0);
						if (now < 0 || !CHAR_IS_NEWLINE(now))
							break;
						else stream_in(stream, 1);
					}
					break;
				}
				else stream_in(stream, 1);
			}
			continue;
		}

		if (now == TOKEN_CHAR_QUOTE)
		{
			if (token_len > 0)
			{
				type = TOKEN_SYMBOL;
				*result = xstring_from_cstr(token_buf, token_len);
				break;
			}
			else
			{
				stream_in(stream, 1);
				type = TOKEN_STRING;
				
				while (1)
				{
					now = stream_in(stream, 1);
					if (now < 0 || now == TOKEN_CHAR_QUOTE)
					{
						if (token_len > 0)
							*result = xstring_from_cstr(token_buf, token_len);
						break;
					}
					else if (now == TOKEN_CHAR_ESCAPE)
					{
						now = stream_in(stream, 1);
						if (now < 0)
						{
							if (token_len > 0)
								*result = xstring_from_cstr(token_buf, token_len);
							break;
						}
						
						++ token_len;
						EXPAND_TOKEN;
						token_buf[token_len - 1] = now;
					}
					else
					{
						++ token_len;
						EXPAND_TOKEN;
						token_buf[token_len - 1] = now;
					}
				}

				break;
			}
		}

		++ token_len;
		EXPAND_TOKEN;
		token_buf[token_len - 1] = now;

		stream_in(stream, 1);
	}
#undef EXPAND_TOKEN

	if (token_buf != NULL) free(token_buf);
	return type;
}

/* Internal parser that parse a complete ast expression */
/* fill the result ptr */
/* return 0 means success, -1 means error */
static int
ast_simple_parse_char_stream_internal(stream_in_f stream_in, void *stream, ast_node_t *result)
{
	xstring_t string;
	int type;
	ast_node_t node;
	
	*result = NULL;
	
	if ((type = parse_token(stream_in, stream, &string)) < 0) return -1;
	if (type == TOKEN_EOF)
	{
		*result = NULL;
		return 0;
	}

	switch (type)
	{
	case TOKEN_SYMBOL:
	{
		node = (ast_node_t)malloc(sizeof(struct ast_node_s));
		if (node == NULL) return -1;
		
		node->header.type = AST_SYMBOL;
		node->header.prev = node->header.next = node;
		node->header.priv = NULL;
		
		node->symbol.str  = string;

		node->symbol.type = SYMBOL_NUMERIC;
		/* Process constant as numeric */
		if (!(xstring_len(node->symbol.str) > 1 &&
			  xstring_cstr(node->symbol.str)[0] == TOKEN_CHAR_NUMERIC_CONSTANT))
		{
			int dot = 0;
			int i;
			for (i = 0; i != string->len; ++ i)
			{
				if (i == 0 && string->cstr[i] == TOKEN_CHAR_MINUS && string->len > 1) continue;
				
				if (!CHAR_IS_NUMERIC(string->cstr[i]))
				{
					if (string->cstr[i] == TOKEN_CHAR_DOT)
					{
						if (dot)
							node->symbol.type = SYMBOL_GENERAL;
						else dot = 1;
					}
					else node->symbol.type = SYMBOL_GENERAL;
				}
			}
		}

		break;
	}
		
	case TOKEN_STRING:
	{
		node = (ast_node_t)malloc(sizeof(struct ast_node_s));
		if (node == NULL) return -1;
		
		node->header.type = AST_SYMBOL;
		node->header.prev = node->header.next = node;
		node->header.priv = NULL;

		node->symbol.type = SYMBOL_STRING;
		node->symbol.str  = string;
		break;
	}
		
	case TOKEN_LC:
	{
		node = (ast_node_t)malloc(sizeof(struct ast_node_s));
		if (node == NULL) return -1;
		*result = node;
		
		node->header.type = AST_GENERAL;
		node->header.prev = node->header.next = node;
		node->header.priv = NULL;

		node->general.head = NULL;

		while (1)
		{
			ast_node_t c;
			int r = ast_simple_parse_char_stream_internal(stream_in, stream, &c);
			if (r < 0)
			{
				if (c != NULL) ast_free(c);
				return -1;
			}
			if (c == NULL) break;

			if (node->general.head == NULL)
			{
				node->general.head = c;
				c->header.prev = c->header.next = c;
			}
			else
			{
				c->header.next = node->general.head;
				c->header.prev = c->header.next->header.prev;

				c->header.next->header.prev = c;
				c->header.prev->header.next = c;
			}
		}
		
		break;
	}
		
	case TOKEN_RC:
		*result = NULL;
		return 0;
	}

	*result = node;
	return 0;
}

ast_node_t 
ast_simple_parse_char_stream(stream_in_f stream_in, void *stream)
{
	ast_node_t node;
	if (ast_simple_parse_char_stream_internal(stream_in, stream, &node) < 0)
	{
		if (node != NULL) ast_free(node);
		node = NULL;
	}
	return node;
}
