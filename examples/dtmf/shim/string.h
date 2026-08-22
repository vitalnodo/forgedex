/*
 * Just enough <string.h> for the IOCCC entry.
 *
 * Functions rather than macros, for the reason given in stdio.h:
 * prog.c has a goto label called memcpy.
 */

#ifndef DTMF_SHIM_STRING_H
#define DTMF_SHIM_STRING_H

#include <stddef.h>

void  *memcpy(void *d, const void *s, size_t n);
void  *memmove(void *d, const void *s, size_t n);
void  *memset(void *d, int c, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
char  *strchr(const char *s, int c);

#endif
