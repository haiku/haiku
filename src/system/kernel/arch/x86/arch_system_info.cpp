/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <arch/system_info.h>

#include <string.h>

#include <KernelExport.h>
#include <OS.h>

#include <boot/kernel_args.h>
#include <cpu.h>
#include <kernel.h>
#include <smp.h>


uint32 sCpuType;
int32 sCpuRevision;
int64 sCpuClockSpeed;


static bool
get_cpuid_for(cpuid_info *info, uint32 currentCPU, uint32 eaxRegister,
	uint32 forCPU)
{
	if (currentCPU != forCPU)
		return false;

	get_current_cpuid(info, eaxRegister);
	return true;
}


status_t
get_cpuid(cpuid_info *info, uint32 eaxRegister, uint32 forCPU)
{
	uint32 numCPUs = (uint32)smp_get_num_cpus();
	cpu_status state;

	if (forCPU >= numCPUs)
		return B_BAD_VALUE;

	// prevent us from being rescheduled
	state = disable_interrupts();

	// ToDo: as long as we only run on pentium-class systems, we can assume
	//	that the CPU supports cpuid.

	if (!get_cpuid_for(info, smp_get_current_cpu(), eaxRegister, forCPU)) {
		smp_send_broadcast_ici(SMP_MSG_CALL_FUNCTION, (addr_t)info,
			eaxRegister, forCPU, (void *)get_cpuid_for, SMP_MSG_FLAG_SYNC);
	}

	restore_interrupts(state);
	return B_OK;
}


status_t
arch_get_system_info(system_info *info, size_t size)
{
	info->cpu_type = (cpu_types)sCpuType;
	info->cpu_revision = sCpuRevision;

	// - various cpu_info
	info->cpu_clock_speed = sCpuClockSpeed;
	// - bus_clock_speed
#ifdef __x86_64__
	info->platform_type = B_64_BIT_PC_PLATFORM;
#else
	info->platform_type = B_AT_CLONE_PLATFORM;
#endif

	// ToDo: clock speeds could be retrieved via SMBIOS/DMI
	return B_OK;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	// This is what you get if the CPU vendor is not recognized
	// or the CPU does not support cpuid with eax == 1.
	uint32 base;
	uint32 model = 0;
	cpu_ent *cpu = get_cpu_struct();

	switch (cpu->arch.vendor) {
		case VENDOR_INTEL:
			base = B_CPU_INTEL_x86;
			break;
		case VENDOR_AMD:
			base = B_CPU_AMD_x86;
			break;
		case VENDOR_CYRIX:
			base = B_CPU_CYRIX_x86;
			break;
		case VENDOR_UMC:
			base = B_CPU_INTEL_x86; // XXX
			break;
		case VENDOR_NEXGEN:
			base = B_CPU_INTEL_x86; // XXX
			break;
		case VENDOR_CENTAUR:
			base = B_CPU_VIA_IDT_x86;
			break;
		case VENDOR_RISE:
			base = B_CPU_RISE_x86;
			break;
		case VENDOR_TRANSMETA:
			base = B_CPU_TRANSMETA_x86;
			break;
		case VENDOR_NSC:
			base = B_CPU_NATIONAL_x86;
			break;
		default:
			base = B_CPU_x86;
	}

	if (base != B_CPU_x86) {
		if (base == B_CPU_INTEL_x86
			|| (base == B_CPU_AMD_x86 && cpu->arch.family == 0xF)) {
			model = (cpu->arch.extended_family << 20)
				+ (cpu->arch.extended_model << 16)
				+ (cpu->arch.family << 4) + cpu->arch.model;
		} else {
			model = (cpu->arch.family << 4)
				+ cpu->arch.model;
			// Isn't much useful extended family and model information
			// yet on other processors.
		}
	}

	sCpuRevision = (cpu->arch.extended_family << 18)
		| (cpu->arch.extended_model << 14) | (cpu->arch.type << 12)
		| (cpu->arch.family << 8) | (cpu->arch.model << 4) | cpu->arch.stepping;

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
