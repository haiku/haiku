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


static phys_addr_t sACPIRootPointer = 0;
static phys_addr_t sSMBIOSv2RootPointer = 0;
static phys_addr_t sSMBIOSv3RootPointer = 0;


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

	KMessage bootVolume;
	bootVolume.SetTo(args->boot_volume, args->boot_volume_size);
	sSMBIOSv2RootPointer = bootVolume.GetInt64(BOOT_EFI_SMBIOS_V2_ROOT, 0);
	if (sSMBIOSv2RootPointer != 0) {
		add_boot_item("SMBIOSv2_ROOT_POINTER",
			&sSMBIOSv2RootPointer, sizeof(sSMBIOSv2RootPointer));
	}
	sSMBIOSv3RootPointer = bootVolume.GetInt64(BOOT_EFI_SMBIOS_V3_ROOT, 0);
	if (sSMBIOSv3RootPointer != 0) {
		add_boot_item("SMBIOSv3_ROOT_POINTER",
			&sSMBIOSv3RootPointer, sizeof(sSMBIOSv3RootPointer));
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

