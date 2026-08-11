/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * calculate_cpu_conversion_factor() was written by Travis Geiselbrecht and
 * licensed under the NewOS license.
 */


#include <OS.h>

#include <boot/arch/x86/arch_cpu.h>
#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <arch/cpu.h>
#include <arch/x86/arch_cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>


void
ucode_load(BootVolume& volume)
{
	cpuid_info info;
	if (get_current_cpuid(&info, 0, 0) != B_OK)
		return;

	bool isIntel = strncmp(info.eax_0.vendor_id, "GenuineIntel", 12) == 0;
	bool isAmd = strncmp(info.eax_0.vendor_id, "AuthenticAMD", 12) == 0;

	if (!isIntel && !isAmd)
		return;

	if (get_current_cpuid(&info, 1, 0) != B_OK)
		return;

	char path[128];
	int family = info.eax_1.family;
	int model = info.eax_1.model;
	if (family == 0x6 || family == 0xf) {
		family += info.eax_1.extended_family;
		model += (info.eax_1.extended_model << 4);
	}
	if (isIntel) {
		snprintf(path, sizeof(path), "system/non-packaged/data/firmware/intel-ucode/"
			"%02x-%02x-%02x", family, model, info.eax_1.stepping);
	} else if (family < 0x15) {
		snprintf(path, sizeof(path), "system/non-packaged/data/firmware/amd-ucode/"
			"microcode_amd.bin");
	} else {
		snprintf(path, sizeof(path), "system/non-packaged/data/firmware/amd-ucode/"
			"microcode_amd_fam%02xh.bin", family);
	}
	dprintf("ucode_load: %s\n", path);

	int fd = open_from(volume.RootDirectory(), path, O_RDONLY);
	if (fd < B_OK) {
		dprintf("ucode_load: couldn't find microcode\n");
		return;
	}
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		dprintf("ucode_load: couldn't stat microcode file\n");
		close(fd);
		return;
	}

	ssize_t length = stat.st_size;

	// 16-byte alignment required
	void *buffer = kernel_args_malloc(length, 16);
	if (buffer != NULL) {
		if (read(fd, buffer, length) != length) {
			dprintf("ucode_load: couldn't read microcode file\n");
			kernel_args_free(buffer);
		} else {
			gKernelArgs.ucode_data = buffer;
			gKernelArgs.ucode_data_size = length;
			dprintf("ucode_load: microcode file read in memory\n");
		}
	}

	close(fd);
}


extern "C" status_t
boot_arch_cpu_init()
{
    // Nothing really to init on x86
    return B_OK;
}


extern "C" void
arch_ucode_load(BootVolume& volume)
{
    ucode_load(volume);
}
