/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 */

#include <KernelExport.h>
#include <stdio.h>

#include <cpu.h>
#include <arch_system_info.h>
#include <cpuidle.h>


static status_t std_ops(int32 op, ...);


static inline void
x86_monitor(const void *addr, unsigned long ecx, unsigned long edx)
{
	asm volatile("monitor"
		:: "a" (addr), "c" (ecx), "d"(edx));
}


static inline void
x86_mwait(unsigned long eax, unsigned long ecx)
{
	asm volatile("mwait"
		:: "a" (eax), "c" (ecx));
}


static void *kMwaitEax[] = {
	// C0, we never use it
	(void *)0x00,
	// MWAIT C1
	(void *)0x00,
	// MWAIT C2
	(void *)0x10,
	// MWAIT C3
	(void *)0x20,
	// MWAIT C4
	(void *)0x30,
	// MWAIT C5
	(void *)0x40,
	// MWAIT C6, 0x2 is used to fully shrink L2 cache
	(void *)0x52
};


static int32
IntelCstateIdleEnter(int32 state, CpuidleCstate *cstate)
{
	cpu_ent *cpu = get_cpu_struct();
	if (cpu->invoke_scheduler)
		return 0;

	x86_monitor((void *)&cpu->invoke_scheduler, 0, 0);
	if (!cpu->invoke_scheduler)
		x86_mwait((unsigned long)cstate->pData, 1);

	return state;
}


static CpuidleCstate kSnbcStates[CPUIDLE_CSTATE_MAX] = {
	{},
	{
		"C1-SNB",
		1,
		IntelCstateIdleEnter,
	},
	{
		"C3-SNB",
		80,
		IntelCstateIdleEnter,
	},
	{
		"C6-SNB",
		104,
		IntelCstateIdleEnter,
	},
	{
		"C7-SNB",
		109,
		IntelCstateIdleEnter,
	},
};


static CpuidleModuleInfo sIntelidleModule = {
	{
		"idle/cpuidles/intel_cpuidle/v1",
		0,
		std_ops
	},

};


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			cpuid_info cpuid;
			get_current_cpuid(&cpuid, 5);
			/* ecx[0] monitor/mwait extension supported
			 * ecx[1] support for treating interrupts as break-events for mwait
			 * edx number of sub-states
			 */
			if (!((cpuid.regs.ecx & 0x1) && (cpuid.regs.ecx & 0x2) &&
				cpuid.regs.edx)) {
				return B_ERROR;
			}

			sIntelidleModule.cStateCount = 1;
			for (int32 i = 1; i < CPUIDLE_CSTATE_MAX; i++) {
				int32 subStates = (cpuid.regs.edx >> ((i) * 4)) & 0xf;
				// no sub-states means the state is not available
				if (!subStates)
					continue;
				sIntelidleModule.cStates[sIntelidleModule.cStateCount] =
					kSnbcStates[i];
				sIntelidleModule.cStates[sIntelidleModule.cStateCount].pData =
					kMwaitEax[i];
				sIntelidleModule.cStateCount++;
			}
			dprintf("intel idle init\n");
			return B_OK;
		}
		case B_MODULE_UNINIT:
			dprintf("intel idle uninit\n");
			return B_OK;
	}

	return B_ERROR;
}


module_info *modules[] = {
	(module_info *)&sIntelidleModule,
	NULL
};
