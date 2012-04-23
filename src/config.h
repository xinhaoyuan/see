#ifndef __SEE_CONFIG_H__
#define __SEE_CONFIG_H__

#define SEE_STDLIBC   1
#define SEE_SYSTEM_IO 1
#define SEE_DEBUG     0

/* **************************************** */

#if SEE_STDLIBC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEE_MALLOC  malloc
#define SEE_REALLOC realloc
#define SEE_FREE    free
#define SEE_STRLEN  strlen
#define SEE_MEMCPY  memcpy
#define SEE_MEMCMP  memcmp
#define SEE_FPRINTF fprintf
#define SEE_PRINTF_ERR(v ...) fprintf(stderr, v)
typedef FILE *SEE_FILE_T;

#else

#error Unkonwn configuration

#endif

#if SEE_DEBUG

#define ERROR(ARGS ...)   SEE_FPRINTF(stderr, ARGS)
#define WARNING(ARGS ...) SEE_FPRINTF(stderr, ARGS)

#else

#define ERROR(ARGS ...)
#define WARNING(ARGS ...)

#endif

#endif
