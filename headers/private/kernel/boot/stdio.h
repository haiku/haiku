/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _STDIO_H_
#define _STDIO_H_
	// must match the one of the real stdio.h

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FILE FILE;
	// dummy definition of FILE
	// In the boot loader, it really is a ConsoleNode

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#ifndef SEEK_SET
#	define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#	define SEEK_CUR 1
#endif
#ifndef SEEK_END
#	define SEEK_END 2
#endif

#define EOF -1

#define __PRINTFLIKE(a, b)

extern int printf(char const *format, ...) __PRINTFLIKE(1,2);
extern int fprintf(FILE *stream, char const *format, ...) __PRINTFLIKE(2,3);
extern int sprintf(char *str, char const *format, ...) __PRINTFLIKE(2,3);
extern int snprintf(char *str, size_t size, char const *format, ...) __PRINTFLIKE(3,4);
extern int asprintf(char **ret, char const *format, ...) __PRINTFLIKE(2,3);
extern int vprintf(char const *format, va_list ap);
extern int vfprintf(FILE *stream, char const *format, va_list ap);
extern int vsprintf(char *str, char const *format, va_list ap);
extern int vsnprintf(char *str, size_t size, char const *format, va_list ap);
extern int vasprintf(char **ret, char const *format, va_list ap);

// ToDo: not everything is or should be implemented here
extern int    fgetc(FILE *);
extern char  *fgets(char *, int, FILE *);
extern int    fputc(int, FILE *);
extern int    fputs(const char *, FILE*);
extern int    getc(FILE *);
extern char  *gets(char *);
extern int    getw(FILE *);
extern int    getchar(void);
extern int    putc(int, FILE *);
extern int    putchar(int);
extern int    puts(const char *);
extern int    putw(int, FILE *);

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_H_ */
