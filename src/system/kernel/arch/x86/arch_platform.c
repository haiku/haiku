/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <arch/platform.h>


status_t
arch_platform_init(struct kernel_args *kernelArgs)
{
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return B_OK;
}
