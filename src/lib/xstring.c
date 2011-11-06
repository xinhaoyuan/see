#include <stdlib.h>
#include <string.h>

#include "xstring.h"

xstring_t
xstring_from_cstr(const char *cstr, int len)
{
	if (len < 0) len = strlen(cstr);
	
	xstring_t result = (xstring_t)malloc(sizeof(struct xstring_s));
	result->cstr = (char *)malloc(sizeof(char) * (len + 1));
	result->len = len;

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

void
xstring_free(xstring_t string)
{
	free(string->cstr);
	free(string);
}
