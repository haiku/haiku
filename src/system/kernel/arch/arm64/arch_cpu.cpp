/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <commpage.h>
#include <elf.h>


extern "C" void _exception_vectors(void);


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	WRITE_SPECIALREG(VBAR_EL1, _exception_vectors);
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	uint64_t tcr = READ_SPECIALREG(TCR_EL1);
	uint64_t mmfr1 = READ_SPECIALREG(ID_AA64MMFR1_EL1);

	uint64_t hafdbs = ID_AA64MMFR1_HAFDBS(mmfr1);
	if (hafdbs == ID_AA64MMFR1_HAFDBS_AF) {
		tcr |= (1UL << 39);
	}
	if (hafdbs == ID_AA64MMFR1_HAFDBS_AF_DBS) {
		tcr |= (1UL << 40) | (1UL << 39);
	}

	tcr |= TCR_SH1_IS | TCR_IRGN1_WBWA | TCR_ORGN1_WBWA;
	tcr |= TCR_SH0_IS | TCR_IRGN0_WBWA | TCR_ORGN0_WBWA;
	tcr &= ~TCR_T0SZ(0x1f);
	tcr |= TCR_T0SZ(16);

	WRITE_SPECIALREG(TCR_EL1, tcr);

	return 0;
}


status_t
arch_cpu_init(kernel_args *args)
{
	for (uint32 i = 0; i < args->num_cpus; i++) {
		cpu_ent* cpu = &gCPU[i];

		cpu->topology_id[CPU_TOPOLOGY_PACKAGE] = 0;
		cpu->topology_id[CPU_TOPOLOGY_CORE] = i;
		cpu->topology_id[CPU_TOPOLOGY_SMT] = 0;
	}
	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args *args)
{
	return B_OK;
}


status_t
arch_cpu_shutdown(bool reboot)
{
	// never reached
	return B_ERROR;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
	uint64_t ctr_el0 = 0;
	asm volatile ("mrs\t%0, ctr_el0":"=r" (ctr_el0));

	uint64_t icache_line_size = 4 << (ctr_el0 << 0xF);
	uint64_t dcache_line_size = 4 << ((ctr_el0 >> 16) & 0xF);
	uint64_t addr = (uint64_t)address;
	uint64_t end = addr + len;

	for (uint64_t address_dcache = ROUNDDOWN(addr, dcache_line_size);
	     address_dcache < end; address_dcache += dcache_line_size) {
		asm volatile ("dc cvau, %0" : : "r"(address_dcache) : "memory");
	}

	asm("dsb ish");

	for (uint64_t address_icache = ROUNDDOWN(addr, icache_line_size);
         address_icache < end; address_icache += icache_line_size) {
		asm volatile ("ic ivau, %0" : : "r"(address_icache) : "memory");
	}
	asm("dsb ish");
	asm("isb");
}


void
arch_cpu_invalidate_tlb_range(intptr_t, addr_t start, addr_t end)
{
	arch_cpu_global_tlb_invalidate();
}


void
arch_cpu_invalidate_tlb_list(intptr_t, addr_t pages[], int num_pages)
{
	arch_cpu_global_tlb_invalidate();
}


void
arch_cpu_global_tlb_invalidate()
{
	asm(
		"dsb ishst\n"
		"tlbi vmalle1\n"
		"dsb ish\n"
		"isb\n"
	);
}


void
arch_cpu_user_tlb_invalidate(intptr_t)
{
	arch_cpu_global_tlb_invalidate();
}
