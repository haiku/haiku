/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <arch/platform.h>
#include <boot/kernel_args.h>


status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	// NOTE: dprintf() is off-limits here, too early...

	return B_OK;
}

status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	// no area to create, since we pass the FDT in the kernel_args
	// and the VM automagically creates the area's (with B_EXACT_ADDRESS)
	// for them.
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	// now we can create and use semaphores
	return B_OK;
}
