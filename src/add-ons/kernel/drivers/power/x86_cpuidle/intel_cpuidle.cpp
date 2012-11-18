/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 */

#include <KernelExport.h>
#include <cpu.h>
#include <arch_system_info.h>

#include <stdio.h>

#include <cpuidle.h>

#include "x86_cpuidle.h"

static CpuidleDevice sIntelDevice;

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
IntelCstateIdleEnter(int32 state, CpuidleDevice *device)
{
	cpu_ent *cpu = get_cpu_struct();
	if (cpu->invoke_scheduler)
		return 0;

	CpuidleCstate *cState = &device->cStates[state];
	x86_monitor((void *)&cpu->invoke_scheduler, 0, 0);
	if (!cpu->invoke_scheduler)
		x86_mwait((unsigned long)cState->pData, 1);

	return state;
}


static CpuidleCstate sSnbcStates[CPUIDLE_CSTATE_MAX] = {
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


status_t
intel_cpuidle_init(void)
{
	dprintf("intel idle init\n");
	cpu_ent *cpu = get_cpu_struct();
	if (cpu->arch.vendor != VENDOR_INTEL ||	cpu->arch.family != 6)
		return B_ERROR;

	// Calculated Model Value: M = (Extended Model << 4) + Model
	uint32 model = (cpu->arch.extended_model << 4) + cpu->arch.model;
	if (model != 0x2a && model != 0x2d)
		return B_ERROR;

	cpuid_info cpuid;
	get_current_cpuid(&cpuid, 5);
	/* ecx[0] monitor/mwait extension supported
	 * ecx[1] support for treating interrupts as break-events for mwait
	 * edx number of sub-states
	 */
	if ((cpuid.regs.ecx & 0x1) == 0 ||
		(cpuid.regs.ecx & 0x2) == 0 ||
		cpuid.regs.edx == 0) {
		return B_ERROR;
	}

	sIntelDevice.cStateCount = 1;
	for (int32 i = 1; i < CPUIDLE_CSTATE_MAX; i++) {
		int32 subStates = (cpuid.regs.edx >> ((i) * 4)) & 0xf;
		// no sub-states means the state is not available
		if (!subStates)
			continue;
		sIntelDevice.cStates[sIntelDevice.cStateCount] =
			sSnbcStates[i];
		sIntelDevice.cStates[sIntelDevice.cStateCount].pData =
			kMwaitEax[i];
		sIntelDevice.cStateCount++;
	}
	status_t status = gIdle->AddDevice(&sIntelDevice);
	if (status == B_OK)
		dprintf("using intel idle\n");
	return status;
}
