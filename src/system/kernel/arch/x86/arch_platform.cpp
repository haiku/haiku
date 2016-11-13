/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <arch/platform.h>
#include <apm.h>
#include <boot_item.h>
#include <boot/stage2.h>


status_t
arch_platform_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *args)
{
	// Now we can add boot items; pass on the ACPI root pointer
	add_boot_item("ACPI_ROOT_POINTER",
		args->arch_args.acpi_root.Pointer(), sizeof(void*));

	return B_OK;
}


status_t
arch_platform_init_post_thread(struct kernel_args *args)
{
	// APM is not supported on x86_64.
#ifndef __x86_64__
	apm_init(args);
#endif
	return B_OK;
}

