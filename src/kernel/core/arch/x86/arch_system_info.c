/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>

#include <string.h>


extern void cpuid(uint32 selector, cpuid_info *info);
extern uint32 cv_factor;


cpu_type sCpuType;
int32 sCpuRevision;
int64 sCpuClockSpeed;


status_t
get_cpuid(cpuid_info *info, uint32 eax_register, uint32 cpu_num)
{
	// ToDo: cpu_num is ignored, we should use the call_all_cpus() function
	//		to fix this.

	cpuid(eax_register, info);

	if (eax_register == 0) {
		// fixups for this special case
		char *vendor = info->eax_0.vendorid;
		uint32 data[4];

		memcpy(data, info, sizeof(uint32) * 4);

		// the high-order word is non-zero on some Cyrix CPUs
		info->eax_0.max_eax = data[0] & 0xffff;

		// the vendor string bytes need a little re-ordering
		*((uint32 *)&vendor[0]) = data[1];
		*((uint32 *)&vendor[4]) = data[3];
		*((uint32 *)&vendor[8]) = data[2];
	}

	return B_OK;
}


status_t
arch_get_system_info(system_info *info, size_t size)
{
	info->cpu_type = sCpuType;
	info->cpu_revision = sCpuRevision;

	// - various cpu_info
	info->cpu_clock_speed = sCpuClockSpeed;
	// - bus_clock_speed
	info->platform_type = B_AT_CLONE_PLATFORM;

	// ToDo: clock speeds could be retrieved via SMBIOS/DMI
	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	cpuid_info cpuInfo;
	if (get_cpuid(&cpuInfo, 0, 0) == B_OK) {
		int32 base;

		// set CPU type and revision

		if (!strncmp(cpuInfo.eax_0.vendorid, "GenuineIntel", 12))
			base = B_CPU_INTEL_X86;
		else if (!strncmp(cpuInfo.eax_0.vendorid, "AuthenticAMD", 12))
			base = B_CPU_AMD_X86;
		else {
			// ToDo: fix me, add Transmeta, VIA, ...!
			base = B_CPU_INTEL_X86;
		}

		get_cpuid(&cpuInfo, 1, 0);

		sCpuType = base + cpuInfo.eax_1.model + (cpuInfo.eax_1.family << 4);
		sCpuRevision = (cpuInfo.eax_1.type << 12)
			| (cpuInfo.eax_1.family << 8)
			| (cpuInfo.eax_1.model << 4)
			| cpuInfo.eax_1.stepping;
	} else {
		sCpuType = B_CPU_INTEL_X86;
	}

	sCpuClockSpeed = args->arch_args.cpu_clock_speed;
	return B_OK;
}

