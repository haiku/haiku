/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "x86_syscalls.h"

#include <KernelExport.h>

#ifdef _COMPAT_MODE
#	include <commpage_compat.h>
#endif
#include <cpu.h>
#ifdef _COMPAT_MODE
#	include <elf.h>
#endif
#include <smp.h>


// SYSCALL handler (in interrupts.S).
extern "C" void x86_64_syscall_entry(void);


#ifdef _COMPAT_MODE

// SYSCALL/SYSENTER handlers (in entry_compat.S).
extern "C" {
	void x86_64_syscall32_entry(void);
	void x86_64_sysenter32_entry(void);
}


void (*gX86SetSyscallStack)(addr_t stackTop) = NULL;


// user syscall assembly stubs
extern "C" void x86_user_syscall_sysenter(void);
extern unsigned int x86_user_syscall_sysenter_end;
extern "C" void x86_user_syscall_syscall(void);
extern unsigned int x86_user_syscall_syscall_end;

extern "C" void x86_sysenter32_userspace_thread_exit(void);
extern unsigned int x86_sysenter32_userspace_thread_exit_end;

#endif // _COMPAT_MODE


static void
init_syscall_registers(void* dummy, int cpuNum)
{
	// Enable SYSCALL (EFER.SCE = 1).
	x86_write_msr(IA32_MSR_EFER, x86_read_msr(IA32_MSR_EFER)
		| IA32_MSR_EFER_SYSCALL);

	// Flags to clear upon entry. Want interrupts disabled and the carry, trap,
	// nested task, direction and alignment check flags cleared.
	x86_write_msr(IA32_MSR_FMASK, X86_EFLAGS_INTERRUPT | X86_EFLAGS_DIRECTION
		| X86_EFLAGS_NESTED_TASK | X86_EFLAGS_CARRY | X86_EFLAGS_TRAP
		| X86_EFLAGS_ALIGNMENT_CHECK);

	// Entry point address.
	x86_write_msr(IA32_MSR_LSTAR, (addr_t)x86_64_syscall_entry);

#ifdef _COMPAT_MODE
	// Syscall compat entry point address.
	x86_write_msr(IA32_MSR_CSTAR, (addr_t)x86_64_syscall32_entry);
#endif

	// Segments that will be set upon entry and return. This is very strange
	// and requires a specific ordering of segments in the GDT. Upon entry:
	//  - CS is set to IA32_STAR[47:32]
	//  - SS is set to IA32_STAR[47:32] + 8
	// Upon return:
	//  - CS is set to IA32_STAR[63:48] + 16
	//  - SS is set to IA32_STAR[63:48] + 8
	// From this we get:
	//  - Entry CS  = KERNEL_CODE_SELECTOR
	//  - Entry SS  = KERNEL_CODE_SELECTOR + 8  = KERNEL_DATA_SELECTOR
	//  - Return CS = USER32_CODE_SELECTOR + 16 = USER_CODE_SELECTOR
	//  - Return SS = USER32_CODE_SELECTOR + 8  = USER_DATA_SELECTOR
	x86_write_msr(IA32_MSR_STAR, ((uint64)(USER32_CODE_SELECTOR) << 48)
		| ((uint64)(KERNEL_CODE_SELECTOR) << 32));
}


// #pragma mark -


void
x86_initialize_syscall(void)
{
	// SYSCALL/SYSRET are always available on x86_64 so we just use them, no
	// need to use the commpage. Tell all CPUs to initialize the SYSCALL MSRs.
	call_all_cpus_sync(&init_syscall_registers, NULL);
}


#ifdef _COMPAT_MODE

static void
set_intel_syscall_stack(addr_t stackTop)
{
	x86_write_msr(IA32_MSR_SYSENTER_ESP, stackTop);
}


static void
init_intel_syscall_registers(void* dummy, int cpuNum)
{
	x86_write_msr(IA32_MSR_SYSENTER_CS, KERNEL_CODE_SELECTOR);
	x86_write_msr(IA32_MSR_SYSENTER_ESP, 0);
	x86_write_msr(IA32_MSR_SYSENTER_EIP, (addr_t)x86_64_sysenter32_entry);

	gX86SetSyscallStack = &set_intel_syscall_stack;
}


void
x86_compat_initialize_syscall(void)
{
	// for 32-bit syscalls, fill the commpage with the right mechanism
	call_all_cpus_sync(&init_intel_syscall_registers, NULL);

	void* syscallCode = (void *)&x86_user_syscall_sysenter;
	void* syscallCodeEnd = &x86_user_syscall_sysenter_end;

	// TODO check AMD for sysenter

	// fill in the table entry
	size_t len = (size_t)((addr_t)syscallCodeEnd - (addr_t)syscallCode);
	addr_t position = fill_commpage_compat_entry(COMMPAGE_ENTRY_X86_SYSCALL,
		syscallCode, len);

	image_id image = get_commpage_compat_image();
	elf_add_memory_image_symbol(image, "commpage_compat_syscall", position,
		len, B_SYMBOL_TYPE_TEXT);

	void* threadExitCode = (void *)&x86_sysenter32_userspace_thread_exit;
	void* threadExitCodeEnd = &x86_sysenter32_userspace_thread_exit_end;

	len = (size_t)((addr_t)threadExitCodeEnd - (addr_t)threadExitCode);
	position = fill_commpage_compat_entry(COMMPAGE_ENTRY_X86_THREAD_EXIT,
		threadExitCode, len);

	elf_add_memory_image_symbol(image, "commpage_compat_thread_exit",
		position, len, B_SYMBOL_TYPE_TEXT);
}

#endif // _COMPAT_MODE

