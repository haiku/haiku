/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
 * Copyright 2006 Ingo Weinhold, bonefish@cs.tu-berlin.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */

//#include <arch_platform.h>

#include <new>

#include <KernelExport.h>
#include <arch/platform.h>
#include <boot/kernel_args.h>
#include <real_time_clock.h>
#include <util/kernel_cpp.h>


status_t
arch_platform_init(struct kernel_args* kernelArgs)
{
#warning IMPLEMENT arch_platform_init
	return B_ERROR;
}


status_t
arch_platform_init_post_vm(struct kernel_args* kernelArgs)
{
#warning IMPLEMENT arch_platform_init_post_vm
	return B_ERROR;
}


status_t
arch_platform_init_post_thread(struct kernel_args* kernelArgs)
{
#warning IMPLEMENT arch_platform_init_post_thread
	return B_ERROR;
}

