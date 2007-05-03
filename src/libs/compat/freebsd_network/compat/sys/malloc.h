#ifndef _FBSD_COMPAT_SYS_MALLOC_H_
#define _FBSD_COMPAT_SYS_MALLOC_H_

#include <malloc.h>

#define M_NOWAIT		0x0001
#define M_WAITOK		0x0002
#define M_ZERO			0x0100


void *_kernel_malloc(size_t size, int flags);
void _kernel_free(void *ptr);

#define kernel_malloc(size, base, flags) \
	_kernel_malloc(size, flags)

#define kernel_free( ptr, base) \
	_kernel_free(ptr)

#endif
