/*
 * Copyright 2012-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Yongcong Du <ycdu.vmcore@gmail.com>
 *		Paweł Dziepak, <pdziepak@quarnos.org>
 */


#include <arch_system_info.h>
#include <cpu.h>
#include <debug.h>
#include <smp.h>
#include <thread.h>
#include <util/AutoLock.h>

#include <cpuidle.h>
#include <KernelExport.h>

#include <stdio.h>


#define CPUIDLE_CSTATE_MAX			8

#define MWAIT_INTERRUPTS_BREAK		(1 << 0)

#define INTEL_CSTATES_MODULE_NAME	CPUIDLE_MODULES_PREFIX "/intel_cstates/v1"

#define BASE_TIME_STEP				500

struct CState {
	uint32	fCode;
	int		fSubStatesCount;
};

static CState sCStates[CPUIDLE_CSTATE_MAX];
static int sCStateCount;

static int sTimeStep = BASE_TIME_STEP;
static bool sEnableWait = false;

static bigtime_t* sIdleTime;


static inline void
x86_monitor(void* address, uint32 ecx, uint32 edx)
{
	asm volatile("monitor" : : "a" (address), "c" (ecx), "d"(edx));
}


static inline void
x86_mwait(uint32 eax, uint32 ecx)
{
	asm volatile("mwait" : : "a" (eax), "c" (ecx));
}


static void
cstates_set_scheduler_mode(scheduler_mode mode)
{
	if (mode == SCHEDULER_MODE_POWER_SAVING) {
		sTimeStep = BASE_TIME_STEP / 4;
		sEnableWait = true;
	} else {
		sTimeStep = BASE_TIME_STEP;
		sEnableWait = false;
	}
}


static void
cstates_idle(void)
{
	ASSERT(thread_get_current_thread()->pinned_to_cpu > 0);
	int32 cpu = smp_get_current_cpu();

	bigtime_t timeStep = sTimeStep;
	bigtime_t idleTime = sIdleTime[cpu];
	int state = min_c(idleTime / timeStep, sCStateCount - 1);

	ASSERT(state >= 0 && state < sCStateCount);

	int subState = idleTime % timeStep;
	subState *= sCStates[state].fSubStatesCount;
	subState /= timeStep;

	ASSERT(subState >= 0 && subState < sCStates[state].fSubStatesCount);

	InterruptsLocker locker;
	int dummy;
	bigtime_t start = system_time();
	x86_monitor(&dummy, 0, 0);
	x86_mwait(sCStates[state].fCode | subState, MWAIT_INTERRUPTS_BREAK);
	bigtime_t delta = system_time() - start;
	locker.Unlock();

	sIdleTime[cpu] = (idleTime + delta) / 2;
}


static void
cstates_wait(int32* variable, int32 test)
{
	if (!sEnableWait)
		return;

	InterruptsLocker _;
	x86_monitor(variable, 0, 0);
	if (*variable != test)
		x86_mwait(sCStates[0].fCode, MWAIT_INTERRUPTS_BREAK);
}


static status_t
init_cstates()
{
	if (!x86_check_feature(IA32_FEATURE_EXT_MONITOR, FEATURE_EXT))
		return B_ERROR;
	if (!x86_check_feature(IA32_FEATURE_POWER_MWAIT, FEATURE_5_ECX))
		return B_ERROR;
	if (!x86_check_feature(IA32_FEATURE_INTERRUPT_MWAIT, FEATURE_5_ECX))
		return B_ERROR;

	// we need invariant TSC
	if (!x86_check_feature(IA32_FEATURE_INVARIANT_TSC, FEATURE_EXT_7_EDX))
		return B_ERROR;

	// get C-state data
	cpuid_info cpuid;
	get_current_cpuid(&cpuid, 0, 0);
	uint32 maxBasicLeaf = cpuid.eax_0.max_eax;
	if (maxBasicLeaf < 5)
		return B_ERROR;

	get_current_cpuid(&cpuid, 5, 0);
	if ((cpuid.regs.eax & 0xffff) < sizeof(int32))
		return B_ERROR;

	char cStates[64];
	unsigned int offset = 0;
	for (int32 i = 1; i < CPUIDLE_CSTATE_MAX; i++) {
		int32 subStates = (cpuid.regs.edx >> (i * 4)) & 0xf;
		// no sub-states means the state is not available
		if (subStates == 0)
			continue;

		if (offset < sizeof(cStates)) {
			offset += snprintf(cStates + offset, sizeof(cStates) - offset,
					", C%" B_PRId32, i);
		}

		sCStates[sCStateCount].fCode = sCStateCount * 0x10;
		sCStates[sCStateCount].fSubStatesCount = subStates;
		sCStateCount++;
	}

	if (sCStateCount == 0)
		return B_ERROR;

	sIdleTime = new(std::nothrow) bigtime_t[smp_get_num_cpus()];
	if (sIdleTime == NULL)
		return B_NO_MEMORY;
	memset(sIdleTime, 0, sizeof(bigtime_t) * smp_get_num_cpus());

	cstates_set_scheduler_mode(SCHEDULER_MODE_LOW_LATENCY);

	dprintf("using Intel C-States: C0%s\n", cStates);
	return B_OK;
}


static status_t
uninit_cstates()
{
	delete[] sIdleTime;
	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_cstates();

		case B_MODULE_UNINIT:
			uninit_cstates();
			return B_OK;
	}

	return B_ERROR;
}


static cpuidle_module_info sIntelCStates = {
	{
		INTEL_CSTATES_MODULE_NAME,
		0,
		std_ops,
	},

	0.8f,

	cstates_set_scheduler_mode,

	cstates_idle,
	cstates_wait
};


module_info* modules[] = {
	(module_info*)&sIntelCStates,
	NULL
};

