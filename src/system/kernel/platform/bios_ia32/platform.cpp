/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <platform.h>

#include <boot/kernel_args.h>

status_t
platform_init(struct kernel_args *kernelArgs)
{
	return B_OK;
}


status_t
platform_init_post_vm(struct kernel_args *kernelArgs)
{
	return B_OK;
}
