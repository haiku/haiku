/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__dl_functions__hh__
#define __newos__dl_functions__hh__


#ifdef __cplusplus
extern "C"
{
#endif


#define RTLD_LAZY   0
#define RTLD_NOW    1
#define RLTD_LOCAL  0
#define RLTD_GLOBAL 2

void *dlopen(char const *, unsigned);
int   dlclose(void *);
void *dlsym(void *, char const *);


#ifdef _cplusplus
} /* "C" */
#endif

#endif
