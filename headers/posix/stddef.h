#ifndef _STDDEF_H_
#define _STDDEF_H_
/* 
** Distributed under the terms of the OpenBeOS License.
*/
#include <null.h>

#define offsetof(type,member) ((size_t)&((type*)0)->member)

typedef long ptrdiff_t;

#include <wchar_t.h>
#include <size_t.h>

#endif	/* _STDDEF_H_ */
