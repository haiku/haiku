#ifndef _FBSD_COMPAT_SYS_MALLOC_H_
#define _FBSD_COMPAT_SYS_MALLOC_H_

#include <malloc.h>

#include <vm/vm.h>

#define M_NOWAIT		0x0001
#define M_WAITOK		0x0002
#define M_ZERO			0x0100


void *_kernel_malloc(size_t size, int flags);
void _kernel_free(void *ptr);

void *_kernel_contigmalloc(const char *file, int line, size_t size, int flags,
	vm_paddr_t low, vm_paddr_t high, unsigned long alignment,
	unsigned long boundary);
void _kernel_contigfree(void *addr, unsigned long size);

#define kernel_malloc(size, base, flags) \
	_kernel_malloc(size, flags)

#define kernel_free( ptr, base) \
	_kernel_free(ptr)

#define kernel_contigmalloc(size, base, flags, low, high, alignment, boundary) \
	_kernel_contigmalloc(__FILE__, __LINE__, size, flags, low, high, \
		alignment, boundary)

#define kernel_contigfree(addr, size, base) \
	_kernel_contigfree(addr, size)

#endif
