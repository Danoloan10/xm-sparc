#ifndef _STD_C_H_
#define _STD_C_H_

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v,l)

void *memset (void *dst, int s, unsigned int count);
void *memcpy(void* dst, const void* src, unsigned int count);
int memcmp(const void *dst, const void *src, unsigned int count);
unsigned int strlen(const char *s);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
char *strchr(const char *t, int c);

int xprintf(char const *fmt, ...);
int sprintf(char *str, char const *fmt, ...);
int vsprintf(char *str, char const *fmt, va_list arg);

#endif
