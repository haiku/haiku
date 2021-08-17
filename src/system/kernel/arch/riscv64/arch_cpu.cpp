/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <vm/VMAddressSpace.h>
#include <commpage.h>
#include <elf.h>
#include <Htif.h>
#include <platform/sbi/sbi_syscalls.h>


extern uint32 gPlatform;


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	//detect_cpu(curr_cpu);

	// we only support one anyway...
	return 0;
}


status_t
arch_cpu_init(kernel_args *args)
{
/*
	uint64 conversionFactor
		= (1LL << 32) * 1000000LL / args->arch_args.timerFrequency;

	__riscv64_setup_system_time(conversionFactor);
*/
	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	// Set address space ownership to currently running threads
	for (uint32 i = 0; i < args->num_cpus; i++) {
		VMAddressSpace::Kernel()->Get();
	}

	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args *args)
{
	return B_OK;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
}


void
arch_cpu_memory_read_barrier(void)
{
}


void
arch_cpu_memory_write_barrier(void)
{
}


void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
}


void
arch_cpu_global_TLB_invalidate(void)
{
}


void
arch_cpu_user_TLB_invalidate(void)
{
}


status_t
arch_cpu_shutdown(bool reboot)
{
	if (gPlatform == kPlatformSbi) {
		sbi_system_reset(
			reboot ? SBI_RESET_TYPE_COLD_REBOOT : SBI_RESET_TYPE_SHUTDOWN,
			SBI_RESET_REASON_NONE);
	}

	HtifShutdown();
	return B_ERROR;
}
