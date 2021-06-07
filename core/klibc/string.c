/*
 * $FILE: string.c
 *
 * String related functions
 *
 * $VERSION$
 *
 * Author: Miguel Masmano <mmasmano@ai2.upv.es>
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <stdc.h>
#include <arch/xm_def.h>

void *memset(void *dst, xm_s32_t s, xmSize_t count) {
    register xm_s8_t *a=dst;
    count++;
    while (--count)
	*a++=s;
    return dst;
}

#ifndef __ARCH_MEMCPY

void *memcpy(void *dst, const void *src, xmSize_t count) {
    register xm_s8_t *d=dst;
    register const xm_s8_t *s=src;

    ++count;
    while (--count) {
	*d=*s;
	++d; ++s;
    }
    return dst;
}

void *MemCpyPhys(void *dst, const void *src, xm_u32_t count) {
    return 0;
}

#endif

char *strncpy(char *dest, const char *src, xmSize_t n) {
    xm_s32_t j;

    memset(dest,0,n);

    for (j=0; j<n && src[j]; j++)
	dest[j]=src[j];

    if (j>=n)
	dest[n-1]=0;

    return dest;
}

xm_s32_t strcmp(const char *s, const char *t) {
    char x;

    for (;;) {
	x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
    }
    return ((xm_s32_t)x)-((xm_s32_t)*t);
}

xm_s32_t strncmp(const char *s1, const char *s2, xmSize_t n) {
    register const xm_u8_t *a=(const xm_u8_t*)s1;
    register const xm_u8_t *b=(const xm_u8_t*)s2;
    register const xm_u8_t *fini=a+n;

    while (a < fini) {
	register xm_s32_t res = *a-*b;
	if (res) return res;
	if (!*a) return 0;
	++a; ++b;
    }
    return 0;
}

xmSize_t strlen(const char *s) {
    xm_u32_t i;
    if (!s) return 0;
    for (i = 0; *s; ++s) ++i;
    return i;
}
