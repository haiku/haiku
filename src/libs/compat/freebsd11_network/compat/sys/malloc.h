/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_MALLOC_H_
#define _FBSD_COMPAT_SYS_MALLOC_H_


#include <malloc.h>

#include <vm/vm.h>

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/_mutex.h>


/*
 * flags to malloc.
 */
#define M_NOWAIT		0x0001
#define M_WAITOK		0x0002
#define M_ZERO			0x0100

#define	M_MAGIC			877983977	/* time when first defined :-) */

#define M_DEVBUF		0


struct malloc_type {
};


void *_kernel_malloc(size_t size, int flags);
void _kernel_free(void *ptr);

void *_kernel_contigmalloc(const char *file, int line, size_t size, int flags,
	vm_paddr_t low, vm_paddr_t high, unsigned long alignment,
	unsigned long boundary);
void _kernel_contigfree(void *addr, unsigned long size);

#define kernel_malloc(size, base, flags) \
	_kernel_malloc(size, flags)

#define kernel_free(ptr, base) \
	_kernel_free(ptr)

#define kernel_contigmalloc(size, type, flags, low, high, alignment, boundary) \
	_kernel_contigmalloc(__FILE__, __LINE__, size, flags, low, high, \
		alignment, boundary)

#define kernel_contigfree(addr, size, base) \
	_kernel_contigfree(addr, size)

#ifdef FBSD_DRIVER
#	define malloc(size, tag, flags)	kernel_malloc(size, tag, flags)
#	define free(pointer, tag)		kernel_free(pointer, tag)
#	define contigmalloc(size, type, flags, low, high, alignment, boundary) \
		_kernel_contigmalloc(__FILE__, __LINE__, size, flags, low, high, \
			alignment, boundary)
#	define contigfree(addr, size, base) \
		_kernel_contigfree(addr, size)
#endif

#define	MALLOC_DEFINE(type, shortdesc, longdesc)	int type[1]

#define	MALLOC_DECLARE(type) \
		extern int type[1]

#endif	/* _FBSD_COMPAT_SYS_MALLOC_H_ */
