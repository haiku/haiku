/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__nulibc_stdlib__hh__
#define __newos__nulibc_stdlib__hh__


#include <ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif

int	      atoi(char const *); 
unsigned int  atoui(const char *num);
long          atol(const char *num);
unsigned long atoul(const char *num);

void * malloc(size_t);
void   free(void *);
void * realloc(void *, size_t);
void * reallocf(void *, size_t);
void * calloc(size_t, size_t);
void * memalign(size_t, size_t);
void * valloc(size_t);

char *getenv(char const *);
int   setenv(char const *, char const *, int);
int   putenv(char const *);
void  unsetenv(char const *);

void *bsearch(void const *, void const *, size_t, size_t, int (*) (void const *, void const *));
int   heapsort(void *, size_t , size_t , int (*)(void const *, void const *));
int   mergesort(void *, size_t, size_t, int (*)(void const *, void const *));
void  qsort(void *, size_t, size_t, int (*)(void const *, void const *));
int   radixsort(u_char const **, int, u_char const *, u_int);
int   sradixsort(u_char const **, int, u_char const *, u_int);

int mblen(const char *, size_t);
int mbtowc(int *, const char *, size_t);
int wctomb(char *, int);
size_t mbstowcs(int *, const char*, size_t);
size_t wcstombs(char *, const int*, size_t);

int64  strtoq(const char *, char **, int);
uint64 strtouq(const char *, char **, int);

#define	RAND_MAX	0x7fffffff
int   rand(void);
int   rand_r(unsigned int *ctx);
void  srand(unsigned);
long  random(void);
void  srandom(unsigned long);

void exit(int);
void _exit(int);

#define MB_CUR_MAX 1

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
void * operator new (size_t);
void * operator new[] (size_t);
void operator delete (void *);
void operator delete[] (void *);
#endif


#endif
