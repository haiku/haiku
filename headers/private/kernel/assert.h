/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef __newos__nulibc_assert__hh__
#define __newos__nulibc_assert__hh__


#ifdef __cplusplus
extern "C"
{
#endif


void _assert(char const *, int, char const *);


#ifdef NDEBUG
# define assert(x)
#else
# define assert(x) ( (x) ? (void)0 : _assert(__FILE__, __LINE__, #x) )
#endif


#ifdef __cplusplus
} /* "C" */
#endif


#endif
