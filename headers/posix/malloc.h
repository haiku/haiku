#ifndef _MALLOC_H
#define _MALLOC_H
/* 
** Distributed under the terms of the OpenBeOS License.
*/

#include <sys/types.h>


// ToDo: there are some BeOS specific things missing, most
//	things are only rarely or almost never used, though.
//	The more important missing functionality *and* prototypes
//	are mcheck(), mstats(), and malloc_find_object_address().
//
//	Also, MALLOC_DEBUG is currently not yet supported.
//
//	If you want to implement it, have a look at the BeOS header
//	file malloc.h located in /boot/develop/headers/posix.


#ifdef __cplusplus
extern "C" {
#endif

extern void *malloc(size_t numBytes);
extern void *realloc(void *oldPointer, size_t newSize);
extern void *calloc(size_t numElements, size_t size);
extern void free(void *pointer);
extern void *memalign(size_t alignment, size_t numBytes);
extern void *valloc(size_t numBytes);

#ifdef __cplusplus
}
#endif

#endif /* _MALLOC_H */
