#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <div_t.h>
#include <size_t.h>
#include <null.h>
#include <wchar_t.h>
#include <stddef.h>
#include <limits.h>
#include <sys/types.h>
#include <SupportDefs.h>


#define RAND_MAX      0x7fffffff
#define MB_CUR_MAX    1

#define EXIT_SUCCESS  0
#define EXIT_FAILURE  1


#ifdef  __cplusplus
extern "C" {
#endif

double	atof(const char *str);
int		atoi(const char *str);
long	atol(const char *str);

unsigned int  atoui(const char *num);
unsigned long atoul(const char *num);

double				strtod  (const char *str, char **end);
long				strtol  (const char *str, char **end, int base);
unsigned long		strtoul (const char *str, char **end, int base);
long long			strtoll (const char *str, char **end, int base);
unsigned long long	strtoull(const char *str, char **end, int base);

void	srand(unsigned int seed);
int		rand(void);
int		rand_r(unsigned int *seed);
void	srandom(unsigned int seed);
int		random(void);
char   *initstate(unsigned int seed, char *state, int n);
char   *setstate(char *state);

void   *calloc(size_t nmemb, size_t size);
void	free(void *ptr);
void   *malloc(size_t size);
void   *realloc(void * ptr, size_t size);

void	abort(void);
int		atexit(void (*func)(void));
int		atfork(void (*func)(void));
void	exit(int);

extern  char **environ;
char   *getenv(const char *name);
int 	putenv(const char *string);
int 	setenv(char const *, char const *, int);

char   *realpath(const char *path, char *resolved);

int		system(const char *command);

char   *mktemp(char *name);


typedef int (*_compare_function)(const void*, const void*);

void   *bsearch(const void*, const void*, size_t, size_t, _compare_function);
int   	heapsort(void *, size_t , size_t , _compare_function);
int   	mergesort(void *, size_t, size_t, _compare_function);
void  	qsort(void *, size_t, size_t, _compare_function);
int 	radixsort(u_char const **, int, u_char const *, u_int);
int 	sradixsort(u_char const **, int, u_char const *, u_int);

int		abs(int n);
long	labs(long n);

div_t	div(int numerator,  int denominator);
ldiv_t	ldiv(long numerator, long denominator);

int		mblen(const char *s, size_t n);
int		mbtowc(wchar_t *pwc, const char *s, size_t n);
int		wctomb(char *s, wchar_t wchar);
size_t	mbstowcs(wchar_t *pwcs, const char *s, size_t n);
size_t	wcstombs(char *s, const wchar_t *pwcs, size_t n);

int64	strtoq(const char *, char **, int);
uint64	strtouq(const char *, char **, int);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _STDLIB_H_ */
