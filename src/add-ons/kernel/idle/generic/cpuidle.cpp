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
static CpuidleModuleInfo *sCpuidleModule;

extern void (*gCpuIdleFunc)(void);


/*
 * next cstate selection algorithm is based on NetBSD's
 * it's simple, stupid
 */
static int32
SelectCstate(CpuidleInfo *info)
{
	static const int32 csFactor = 3;

	for (int32 i = sCpuidleModule->cStateCount - 1; i > 0; i--) {
		CpuidleCstate *cState = &sCpuidleModule->cStates[i];
		if (info->cstateSleep > cState->latency * csFactor)
			return i;
	}

	/* Choose C1 if there's no state found */
	return 1;
}


static inline void
EnterCstate(int32 state, CpuidleInfo *info)
{
	CpuidleCstate *cstate = &sCpuidleModule->cStates[state];
	bigtime_t now = system_time();
	if (cstate->EnterIdle(state, cstate) > 0) {
		bigtime_t diff = system_time() - now;
		info->cstateSleep = diff;
		info->stats[state].usageCount++;
		info->stats[state].usageTime += diff;
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
		{
			dprintf("generic cpuidle init\n");
			void *cookie = open_module_list("idle/cpuidles");
			if (cookie == NULL)
				return B_ERROR;

			char name[B_FILE_NAME_LENGTH];
			size_t nameLength = sizeof(name);

			while (read_next_module_name(cookie, name, &nameLength) == B_OK) {
				if (get_module(name, (module_info **)&sCpuidleModule) == B_OK) {
					break;
				}
			}

			close_module_list(cookie);
			if (sCpuidleModule->cStateCount < 2) {
				dprintf("no enough available cstates, exiting...\n");
				put_module(sCpuidleModule->info.name);
				return B_ERROR;
			} else {
				dprintf("using %s\n", sCpuidleModule->info.name);
				gCpuIdleFunc = CpuCstateIdle;
				return B_OK;
			}
		}
		case B_MODULE_UNINIT:
			dprintf("generic cpuidle uninit\n");
			put_module(sCpuidleModule->info.name);
			return B_OK;
	}

	return B_ERROR;
}


static char *
GetIdleStateName(int32 state)
{
	return sCpuidleModule->cStates[state].name;
}


static int32
GetIdleStateCount(void)
{
	return sCpuidleModule->cStateCount;
}


static void
GetIdleStateInfo(int32 cpu, int32 state, CpuidleStat *stat)
{
	memcpy(stat, &sPerCPU[cpu].stats[state], sizeof(CpuidleStat));
}


static GenCpuidle sModule = {
	{
		B_CPUIDLE_MODULE_NAME,
		0,
		std_ops
	},

	GetIdleStateCount,
	GetIdleStateName,
	GetIdleStateInfo
};


module_info *modules[] = {
	(module_info *)&sModule,
	NULL
};
