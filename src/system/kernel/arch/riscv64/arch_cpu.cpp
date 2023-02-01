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


extern "C" void SVec();

extern uint32 gPlatform;


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// dprintf("arch_cpu_preboot_init_percpu(%" B_PRId32 ")\n", curr_cpu);
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	SetStvec((uint64)SVec);
	SstatusReg sstatus{.val = Sstatus()};
	sstatus.ie = 0;
	sstatus.fs = extStatusInitial; // enable FPU
	sstatus.xs = extStatusOff;
	SetSstatus(sstatus.val);
	SetBitsSie((1 << sTimerInt) | (1 << sSoftInt) | (1 << sExternInt));

	return B_OK;
}


status_t
arch_cpu_init(kernel_args *args)
{
	for (uint32 curCpu = 0; curCpu < args->num_cpus; curCpu++) {
		cpu_ent* cpu = &gCPU[curCpu];

		cpu->arch.hartId = args->arch_args.hartIds[curCpu];

		cpu->topology_id[CPU_TOPOLOGY_PACKAGE] = 0;
		cpu->topology_id[CPU_TOPOLOGY_CORE] = curCpu;
		cpu->topology_id[CPU_TOPOLOGY_SMT] = 0;

		for (unsigned int i = 0; i < CPU_MAX_CACHE_LEVEL; i++)
			cpu->cache_id[i] = -1;
	}

	uint64 conversionFactor
		= (1LL << 32) * 1000000LL / args->arch_args.timerFrequency;

	__riscv64_setup_system_time(conversionFactor);

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
	int32 numPages = end / B_PAGE_SIZE - start / B_PAGE_SIZE;
	while (numPages-- >= 0) {
		FlushTlbPage(start);
		start += B_PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	for (int i = 0; i < num_pages; i++)
		FlushTlbPage(pages[i]);
}


void
arch_cpu_global_TLB_invalidate(void)
{
	FlushTlbAll();
}


void
arch_cpu_user_TLB_invalidate(void)
{
	FlushTlbAll();
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
