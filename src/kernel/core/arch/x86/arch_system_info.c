/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <OS.h>

#include <kernel.h>
#include <smp.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>

#include <string.h>


extern void cpuid(uint32 selector, cpuid_info *info);
extern uint32 cv_factor;


uint32 sCpuType;
int32 sCpuRevision;
int64 sCpuClockSpeed;


status_t
get_cpuid(cpuid_info *info, uint32 eaxRegister, uint32 cpuNum)
{
	if (cpuNum >= (uint32)smp_get_num_cpus())
		return B_BAD_VALUE;

	// ToDo: cpu_num is ignored, we should use the call_all_cpus() function
	//		to fix this.

	return get_current_cpuid(info, eaxRegister);
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
	// This is what you get if the CPU vendor is not recognized
	// or the CPU does not support cpuid with eax == 1.
	uint32 base = B_CPU_x86;
	uint32 model = 0;

	cpuid_info cpuInfo;
	if (get_current_cpuid(&cpuInfo, 0) == B_OK) {
		// set CPU type and revision

		if (!strncmp(cpuInfo.eax_0.vendor_id, "GenuineIntel", 12))
			base = B_CPU_INTEL_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "AuthenticAMD", 12))
			base = B_CPU_AMD_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "CyrixInstead", 12))
			base = B_CPU_CYRIX_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "RiseRiseRise", 12))
			base = B_CPU_RISE_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "CentaurHauls", 12))
			base = B_CPU_IDT_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "NexGenDriven", 12))
			// ToDo: add NexGen CPU types
			base = B_CPU_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "GenuineTMx86", 12))
			base = B_CPU_TRANSMETA_x86;
		else if (!strncmp(cpuInfo.eax_0.vendor_id, "Geode by NSC", 12))
			base = B_CPU_NATIONAL_x86;

		if (cpuInfo.eax_0.max_eax >= 1) {
			get_current_cpuid(&cpuInfo, 1);
			if (base != B_CPU_x86)
				model = (cpuInfo.eax_1.family << 4) + cpuInfo.eax_1.model;

			sCpuRevision = (cpuInfo.eax_1.type << 12)
				| (cpuInfo.eax_1.family << 8)
				| (cpuInfo.eax_1.model << 4)
				| cpuInfo.eax_1.stepping;
		}
	}

	sCpuType = base + model;
	sCpuClockSpeed = args->arch_args.cpu_clock_speed;
	return B_OK;
}


//	#pragma mark -


status_t
_user_get_cpuid(cpuid_info *userInfo, uint32 eaxRegister, uint32 cpuNum)
{
	cpuid_info info;
	status_t status;
	
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = get_cpuid(&info, eaxRegister, cpuNum);

	if (status == B_OK
		&& user_memcpy(userInfo, &info, sizeof(cpuid_info)) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}
