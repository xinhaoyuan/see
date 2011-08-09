#ifndef __XSTRING_H__
#define __XSTRING_H__

typedef struct xstring_s
{
	int   len;
	char *cstr;
} *xstring_t;

xstring_t xstring_from_cstr(char *cstr, int len);
int       xstring_equal_cstr(xstring_t string, char *cstr, int len);
int       xstring_equal(xstring_t a, xstring_t b);
char     *xstring_cstr(xstring_t string);
int       xstring_len(xstring_t string);
void      xstring_free(xstring_t string);

#endif
