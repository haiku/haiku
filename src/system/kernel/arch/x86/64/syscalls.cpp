/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "x86_syscalls.h"

#include <KernelExport.h>

#include <cpu.h>
#include <smp.h>


// SYSCALL handler (in interrupts.S).
extern "C" void x86_64_syscall_entry(void);


static void
init_syscall_registers(void* dummy, int cpuNum)
{
	// Enable SYSCALL (EFER.SCE = 1).
	x86_write_msr(IA32_MSR_EFER, x86_read_msr(IA32_MSR_EFER) | (1 << 0));

	// Flags to clear upon entry. Want interrupts disabled and the direction
	// flag cleared.
	x86_write_msr(IA32_MSR_FMASK, X86_EFLAGS_INTERRUPT | X86_EFLAGS_DIRECTION);

	// Entry point address.
	x86_write_msr(IA32_MSR_LSTAR, (addr_t)x86_64_syscall_entry);

	// Segments that will be set upon entry and return. This is very strange
	// and requires a specific ordering of segments in the GDT. Upon entry:
	//  - CS is set to IA32_STAR[47:32]
	//  - SS is set to IA32_STAR[47:32] + 8
	// Upon return:
	//  - CS is set to IA32_STAR[63:48] + 16
	//  - SS is set to IA32_STAR[63:48] + 8
	// From this we get:
	//  - Entry CS  = KERNEL_CODE_SEG
	//  - Entry SS  = KERNEL_CODE_SEG + 8  = KERNEL_DATA_SEG
	//  - Return CS = KERNEL_DATA_SEG + 16 = USER_CODE_SEG
	//  - Return SS = KERNEL_DATA_SEG + 8  = USER_DATA_SEG
	x86_write_msr(IA32_MSR_STAR, ((uint64)(KERNEL_DATA_SEG | 3) << 48)
		| ((uint64)KERNEL_CODE_SEG << 32));
}


// #pragma mark -


void
x86_initialize_syscall(void)
{
	// SYSCALL/SYSRET are always available on x86_64 so we just use them, no
	// need to use the commpage. Tell all CPUs to initialize the SYSCALL MSRs.
	call_all_cpus_sync(&init_syscall_registers, NULL);
}
