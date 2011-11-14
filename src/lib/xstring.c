#include <stdlib.h>
#include <string.h>

#include "xstring.h"

xstring_t
xstring_from_cstr(const char *cstr, int len)
{
	xstring_t result = (xstring_t)malloc(sizeof(struct xstring_s));
	if (result == NULL) return NULL;

	if (len < 0) len = strlen(cstr);	
	result->cstr = (char *)malloc(sizeof(char) * (len + 1));

	if (result->cstr == NULL)
	{
		free(result);
		return NULL;
	}
	
	result->len = len;
	result->ref = 1;

	memcpy(result->cstr, cstr, sizeof(char) * len);
	result->cstr[len] = 0;

	return result;
}

int
xstring_equal_cstr(xstring_t string, const char *cstr, int len)
{
	if (len < 0) len = strlen(cstr);

	if (string->len != len) return 0;
	else return memcmp(string->cstr, cstr, len) == 0;
}

int
xstring_equal(xstring_t a, xstring_t b)
{
	if (a->len != b->len) return 0;
	else return memcmp(a->cstr, b->cstr, a->len) == 0;
}

char *
xstring_cstr(xstring_t string)
{
	return string->cstr;
}

int
xstring_len(xstring_t string)
{
	return string->len;
}

xstring_t
xstring_dup(xstring_t string)
{
	++ string->ref;
	return string;
}

void
xstring_free(xstring_t string)
{
	if (-- string->ref == 0)
	{
		free(string->cstr);
		free(string);
	}
}
