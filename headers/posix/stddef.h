#ifndef _STDDEF_H_
#define _STDDEF_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/
#include <null.h>

#if defined(__GNUC__) && __GNUC__ > 3
#define offsetof(type,member)   __builtin_offsetof(type, member)
#else
#define offsetof(type,member) ((size_t)&((type*)0)->member)
#endif

typedef long ptrdiff_t;

#include <wchar_t.h>
#include <size_t.h>

#endif	/* _STDDEF_H_ */
