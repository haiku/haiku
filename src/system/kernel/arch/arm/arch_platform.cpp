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
#include <real_time_clock.h>
#include <util/kernel_cpp.h>
#include <kernel/debug.h>

status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	// NOTE: dprintf() is off-limits here, too early...
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	// now we can use the heap and create areas
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	// now we can create and use semaphores
	return B_OK;
}
