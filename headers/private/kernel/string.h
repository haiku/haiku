/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__nulibc_string__hh__
#define __newos__nulibc_string__hh__


#include <ktypes.h>


#ifdef __cplusplus
extern "C" {
#endif


void *memchr (void const *, int, size_t);
int   memcmp (void const *, const void *, size_t);
void *memcpy (void *, void const *, size_t);
void *memmove(void *, void const *, size_t);
void *memset (void *, int, size_t);

char       *strcat(char *, char const *);
char       *strchr(char const *, int);
int         strcmp(char const *, char const *);
char       *strcpy(char *, char const *);
char const *strerror(int);
size_t      strlen(char const *);
char       *strncat(char *, char const *, size_t);
int         strncmp(char const *, char const *, size_t);
char       *strncpy(char *, char const *, size_t);
char       *strpbrk(char const *, char const *);
char       *strrchr(char const *, int);
size_t      strspn(char const *, char const *);
char       *strstr(char const *, char const *);
char       *strtok(char *, char const *);


/* non standard */
void  *bcopy(void const *, void *, size_t);
void   bzero(void *, size_t);
size_t strlcat(char *, char const *, size_t);
size_t strlcpy(char *, char const *, size_t);
int    strncasecmp(char const *, char const *, size_t);
int    strnicmp(char const *, char const *, size_t);
size_t strnlen(char const *s, size_t count);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif
