/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include<stdlib.h>

#include <arch/platform.h>
#include <apm.h>
#include <boot_item.h>
#include <boot/stage2.h>
#include <debug.h>

static phys_addr_t sACPIRootPointer = 0;
static  bios_drive_checksum* sCheckSums = NULL;

status_t
arch_platform_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_platform_init_post_vm(struct kernel_args *args)
{
	// Now we can add boot items; pass on the ACPI root pointer
	sACPIRootPointer = args->arch_args.acpi_root.Get();
	add_boot_item("ACPI_ROOT_POINTER",
		&sACPIRootPointer, sizeof(sACPIRootPointer));

	sCheckSums  = (bios_drive_checksum*)malloc(args->platform_args.bios_drive_checksums_size);
	if (sCheckSums != NULL && args->platform_args.bios_drive_checksums_size > 0) {
		memcpy(sCheckSums, args->platform_args.bios_drive_checksums, args->platform_args.bios_drive_checksums_size);
		add_boot_item("BIOS_DRIVES_CHECKSUMS", sCheckSums, args->platform_args.bios_drive_checksums_size);
	}
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

