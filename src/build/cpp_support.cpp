/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <sys/types.h>

struct dl_phdr_info;

extern "C" int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *info, size_t size,
	 void *data), void *data);

#if defined(_BOOT_MODE) || defined(_KERNEL_MODE)


// compiled into the kernel/boot loader

#include "KernelExport.h"

int
dl_iterate_phdr(int (*callback)(struct dl_phdr_info *info, size_t size,
	void *data), void *data)
{
	// This function should never be called, since it is only needed when
	// exception are used, and we don't do that in the kernel. Hence we
	// don't need to implement it.
	panic("dl_iterate_phdr() called\n");

	return 0;
}

 
#else // !( defined(_BOOT_MODE) || defined(_KERNEL_MODE) )


// compiled into libroot.so (well, not yet)

//#include <link.h>

int
dl_iterate_phdr(int (*callback)(struct dl_phdr_info *info, size_t size,
	void *data), void *data)
{
	// TODO: Here we need to implement the function, since we want exceptions
	// in userland.

	return 0;
}


#endif
