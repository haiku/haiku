/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <KernelExport.h>
#include <arch/platform.h>
#include <boot/kernel_args.h>


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


status_t
arch_platform_init_post_thread(struct kernel_args *kernelArgs)
{
	return B_OK;
}
