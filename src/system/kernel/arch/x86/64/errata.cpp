/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

#include <cpu.h>
#include <arch_cpu.h>


static status_t
patch_errata_percpu_amd(int currentCPU, const cpu_ent* cpu)
{
	// There are no errata to patch on a hypervisor, the host will have
	// already taken care of everything for us.
	if (x86_check_feature(IA32_FEATURE_EXT_HYPERVISOR, FEATURE_EXT))
		return B_OK;

	const uint32 family = cpu->arch.family + cpu->arch.extended_family,
		model = (cpu->arch.extended_model << 4) | cpu->arch.model;

	// Family 10h and 12h, Erratum 721:
	//
	// "Under a highly specific and detailed set of internal timing conditions,
	// the processor may incorrectly update the stack pointer after a long
	// series of push and/or near-call instructions, or a long series of pop
	// and/or near-return instructions.
	//
	// https://www.amd.com/system/files/TechDocs/41322_10h_Rev_Gd.pdf
	// https://www.amd.com/system/files/TechDocs/44739_12h_Rev_Gd.pdf
	switch (family) {
	case 0x10:
	case 0x12:
		x86_write_msr(0xc0011029, x86_read_msr(0xc0011029) | 1);
		break;
	}

	// Family 16h ("Jaguar"), Erratum 793:
	//
	// "Under a highly specific and detailed set of internal timing conditions,
	// a locked instruction may trigger a timing sequence whereby the write to
	// a write combined memory type is not flushed, causing the locked instruction
	// to stall indefinitely."
	//
	// https://www.amd.com/system/files/TechDocs/53072_Rev_Guide_16h_Models_30h-3Fh.pdf
	if (family == 0x16 && model <= 0xf) {
		x86_write_msr(0xc0011020, x86_read_msr(0xc0011020) | ((uint64)1 << 15));
	}

	// Family 17h ("Zen") Model 1h
	// https://www.amd.com/system/files/TechDocs/55449_Fam_17h_M_00h-0Fh_Rev_Guide.pdf
	if (family == 0x17 && model == 0x1) {
		// Erratum 1021:
		//
		// "Under a highly specific and detailed set of internal timing
		// conditions, a load operation may incorrectly receive stale data
		// from an older store operation."
		x86_write_msr(0xc0011029, x86_read_msr(0xc0011029) | (1 << 13));

		// Erratum 1033:
		//
		// "Under a highly specific and detailed set of internal timing
		// conditions, a Lock operation may cause the system to hang."
		x86_write_msr(0xc0011020, x86_read_msr(0xc0011020) | (1 << 4));

		// Erratum 1049:
		//
		// "Under a highly specific and detailed set of internal timing
		// conditions, an FCMOV instruction may yield incorrect data if the
		// following sequence of events occurs:
		//  * An FCOMI instruction
		//  * A non-FP instruction that modifies RFLAGS
		//  * An FCMOV instruction."
		x86_write_msr(0xc0011028, x86_read_msr(0xc0011028) | (1 << 4));

		// Erratum 1095:
		//
		// Under a highly detailed and specific set of internal timing
		// conditions, a lock operation may not fence a younger load operation
		// correctly when the following conditions are met:
		//  * SMT (Simultaneous Multithreading) is enabled, and
		//  * a lock operation on memory location A, followed by a load
		//    operation on memory location B are executing on one thread while
		//  * a lock operation on memory location B, followed by a load operation
		//    on memory location A are executing on the second thread on the
		//    same core.
		// This may result in the load operations on both threads incorrectly
		// receiving pre-lock data."
		x86_write_msr(0xc0011020, x86_read_msr(0xc0011020) | ((uint64)1 << 57));
	}

	return B_OK;
}


status_t
__x86_patch_errata_percpu(int currentCPU)
{
	const cpu_ent* cpu = get_cpu_struct();
	if (cpu->arch.vendor == VENDOR_AMD
		|| cpu->arch.vendor == VENDOR_HYGON) {
		return patch_errata_percpu_amd(currentCPU, cpu);
	}
	return B_OK;
}
