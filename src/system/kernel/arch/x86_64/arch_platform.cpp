/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <arch/platform.h>
//#include <apm.h>


status_t
arch_platform_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *args)
{
	//apm_init(args);
	return B_OK;
}

