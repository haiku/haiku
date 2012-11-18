/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 */

#include <KernelExport.h>
#include <stdio.h>
#include <string.h>

#include <smp.h>
#include <cpuidle.h>


static CpuidleInfo sPerCPU[B_MAX_CPU_COUNT];
static CpuidleDevice *sDevice;
extern void (*gCpuIdleFunc)(void);


/*
 * next cstate selection algorithm is based on NetBSD's
 * it's simple, stupid
 */
static int32
SelectCstate(CpuidleInfo *info)
{
	static const int32 csFactor = 3;

	for (int32 i = sDevice->cStateCount - 1; i > 0; i--) {
		CpuidleCstate *cState = &sDevice->cStates[i];
		if (info->cstateSleep > cState->latency * csFactor)
			return i;
	}

	/* Choose C1 if there's no state found */
	return 1;
}


static inline void
EnterCstate(int32 state, CpuidleInfo *info)
{
	CpuidleCstate *cstate = &sDevice->cStates[state];
	bigtime_t now = system_time();
	int32 finalState = cstate->EnterIdle(state, sDevice);
	if (finalState > 0) {
		bigtime_t diff = system_time() - now;
		info->cstateSleep = diff;
		info->stats[finalState].usageCount++;
		info->stats[finalState].usageTime += diff;
	} else {
		info->cstateSleep = 0;
	}
}


static void
CpuCstateIdle(void)
{
	CpuidleInfo *info = &sPerCPU[smp_get_current_cpu()];
	int32 state = SelectCstate(info);
	EnterCstate(state, info);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_ERROR;
}


status_t AddDevice(CpuidleDevice *device)
{
	sDevice = device;
	memory_write_barrier();
	gCpuIdleFunc = CpuCstateIdle;
	return B_OK;
}


static CpuidleModuleInfo sCpuidleModule = {
	{
		B_CPUIDLE_MODULE_NAME,
		0,
		std_ops
	},

	AddDevice,
};


module_info *modules[] = {
	(module_info *)&sCpuidleModule,
	NULL
};
