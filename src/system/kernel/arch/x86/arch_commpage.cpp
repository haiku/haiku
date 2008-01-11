/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <commpage.h>

#include <string.h>

#include <KernelExport.h>

#include <cpu.h>
#include <smp.h>


// user syscall assembly stubs
extern "C" void _user_syscall_int(void);
extern unsigned int _user_syscall_int_end;
extern "C" void _user_syscall_sysenter(void);
extern unsigned int _user_syscall_sysenter_end;

// sysenter handler
extern "C" void x86_sysenter();


static bool
all_cpus_have_feature(enum x86_feature_type type, int feature)
{
	int i;
	int cpuCount = smp_get_num_cpus();

	for (i = 0; i < cpuCount; i++) {
		if (!(gCPU[i].arch.feature[type] & feature))
			return false;
	}

	return true;
}


static void
init_intel_syscall_registers(void* dummy, int cpuNum)
{
	x86_write_msr(IA32_MSR_SYSENTER_CS, KERNEL_CODE_SEG);
	x86_write_msr(IA32_MSR_SYSENTER_ESP, 0);
	x86_write_msr(IA32_MSR_SYSENTER_EIP, (addr_t)x86_sysenter);
}


#if 0
static void
init_amd_syscall_registers(void* dummy, int cpuNum)
{
	// TODO: ...
}
#endif


static status_t
initialize_commpage_syscall(void)
{
	void* syscallCode = &_user_syscall_int;
	void* syscallCodeEnd = &_user_syscall_int_end;

	// check syscall
	if (all_cpus_have_feature(FEATURE_COMMON, IA32_FEATURE_SEP)
		&& !(gCPU[0].arch.family == 6 && gCPU[0].arch.model < 3
			&& gCPU[0].arch.stepping < 3)) {
		// Intel sysenter/sysexit
		dprintf("initialize_commpage_syscall(): sysenter/sysexit supported\n");

		// the code to be used in userland
		syscallCode = &_user_syscall_sysenter;
		syscallCodeEnd = &_user_syscall_sysenter_end;

		// tell all CPUs to init their sysenter/sysexit related registers
		call_all_cpus_sync(&init_intel_syscall_registers, NULL);
	} else if (all_cpus_have_feature(FEATURE_EXT_AMD,
			IA32_FEATURE_AMD_EXT_SYSCALL)) {
		// AMD syscall/sysret
		dprintf("initialize_commpage_syscall(): syscall/sysret supported "
			"-- not yet by Haiku, though");
	} else {
		// no special syscall support
		dprintf("initialize_commpage_syscall(): no special syscall support\n");
	}

	// fill in the table entry
	size_t len = (size_t)((addr_t)syscallCodeEnd - (addr_t)syscallCode);
	fill_commpage_entry(COMMPAGE_ENTRY_X86_SYSCALL, syscallCode, len);

	return B_OK;
}


status_t
arch_commpage_init(void)
{
	// select the optimum syscall mechanism and patch the commpage
	initialize_commpage_syscall();

	return B_OK;
}

