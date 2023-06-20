/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <commpage.h>
#include <elf.h>


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// The current thread must be NULL for all CPUs till we have threads.
	// Some boot code relies on this.
	arch_thread_set_current_thread(NULL);

	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	if (curr_cpu != 0)
		panic("No SMP support on ARM yet!\n");

	return 0;
}


status_t
arch_cpu_init(kernel_args *args)
{
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
	// add the functions to the commpage image
	image_id image = get_commpage_image();

	return B_OK;
}


status_t
arch_cpu_shutdown(bool reboot)
{
	while(1)
		arch_cpu_idle();

	// never reached
	return B_ERROR;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
	uint32 Rd = 0;
	asm volatile ("mcr p15, 0, %[c7format], c7, c5, 0"
		: : [c7format] "r" (Rd) );
}


void
arch_cpu_invalidate_TLB_page(addr_t page)
{
	// ensure visibility of the update to translation table walks
	dsb();

	// TLBIMVAIS(page)
	asm volatile ("mcr p15, 0, %0, c8, c3, 1"
		: : "r" (page));

	// ensure completion of TLB invalidation
	dsb();
	isb();
}

void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	// ensure visibility of the update to translation table walks
	dsb();

	int32 num_pages = end / B_PAGE_SIZE - start / B_PAGE_SIZE;
	while (num_pages-- >= 0) {
		asm volatile ("mcr p15, 0, %[c8format], c8, c6, 1"
			: : [c8format] "r" (start) );
		start += B_PAGE_SIZE;
	}

	// ensure completion of TLB invalidation
	dsb();
	isb();
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	// ensure visibility of the update to translation table walks
	dsb();

	for (int i = 0; i < num_pages; i++) {
		asm volatile ("mcr p15, 0, %[c8format], c8, c6, 1":
			: [c8format] "r" (pages[i]) );
	}

	// ensure completion of TLB invalidation
	dsb();
	isb();
}


void
arch_cpu_global_TLB_invalidate(void)
{
	// ensure visibility of the update to translation table walks
	dsb();

	uint32 Rd = 0;
	asm volatile ("mcr p15, 0, %[c8format], c8, c7, 0"
		: : [c8format] "r" (Rd) );

	// ensure completion of TLB invalidation
	dsb();
	isb();
}


void
arch_cpu_user_TLB_invalidate(void)
{/*
	cpu_ops.flush_insn_pipeline();
	cpu_ops.flush_atc_user();
	cpu_ops.flush_insn_pipeline();
*/
#warning WRITEME
}
