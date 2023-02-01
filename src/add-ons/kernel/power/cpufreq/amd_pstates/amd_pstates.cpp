/*
 * Copyright 2020-2022, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, <pdziepak@quarnos.org>
 */


#include <cpufreq.h>
#include <KernelExport.h>

#include <arch_cpu.h>
#include <cpu.h>
#include <smp.h>
#include <util/AutoLock.h>


#define AMD_PSTATES_MODULE_NAME	CPUFREQ_MODULES_PREFIX "/amd_pstates/v1"


static uint32 sHWPLowest;
static uint32 sHWPGuaranteed;
static uint32 sHWPEfficient;
static uint32 sHWPHighest;

static bool sAvoidBoost = true;


static void set_normal_pstate(void* /* dummy */, int cpu);


static void
pstates_set_scheduler_mode(scheduler_mode mode)
{
	sAvoidBoost = mode == SCHEDULER_MODE_POWER_SAVING;
	call_all_cpus(set_normal_pstate, NULL);
}


static status_t
pstates_increase_performance(int delta)
{
	return B_NOT_SUPPORTED;
}


static status_t
pstates_decrease_performance(int delta)
{
	return B_NOT_SUPPORTED;
}


static bool
is_cpu_model_supported(cpu_ent* cpu)
{
	if (cpu->arch.vendor != VENDOR_AMD)
		return false;

	return true;
}


static void
set_normal_pstate(void* /* dummy */, int cpu)
{
	x86_write_msr(MSR_AMD_CPPC_ENABLE, 1);

	uint64 cap1 = x86_read_msr(MSR_AMD_CPPC_CAP1);
	sHWPLowest = AMD_CPPC_LOWEST_PERF(cap1);
	sHWPEfficient = AMD_CPPC_LOWNONLIN_PERF(cap1);
	sHWPGuaranteed = AMD_CPPC_NOMINAL_PERF(cap1);
	sHWPHighest = AMD_CPPC_HIGHEST_PERF(cap1);

	uint64 request = AMD_CPPC_MIN_PERF(sHWPEfficient);
	request |= AMD_CPPC_MAX_PERF(sAvoidBoost ? sHWPGuaranteed : sHWPHighest);
	request |= AMD_CPPC_EPP_PERF(
		sAvoidBoost ? AMD_CPPC_EPP_BALANCE_PERFORMANCE : AMD_CPPC_EPP_PERFORMANCE);
	x86_write_msr(MSR_AMD_CPPC_REQ, request & 0xffffffff);
}


static status_t
init_pstates()
{
	if (!x86_check_feature(IA32_FEATURE_CPPC, FEATURE_EXT_8_EBX))
		return B_ERROR;

	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++) {
		if (!is_cpu_model_supported(&gCPU[i]))
			return B_ERROR;
	}

	dprintf("using AMD P-States (capabilities: 0x%08" B_PRIx64 "\n",
		x86_read_msr(MSR_AMD_CPPC_CAP1));

	pstates_set_scheduler_mode(SCHEDULER_MODE_LOW_LATENCY);

	call_all_cpus_sync(set_normal_pstate, NULL);
	return B_OK;
}


static status_t
uninit_pstates()
{
	call_all_cpus_sync(set_normal_pstate, NULL);

	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_pstates();

		case B_MODULE_UNINIT:
			uninit_pstates();
			return B_OK;
	}

	return B_ERROR;
}


static cpufreq_module_info sAMDPStates = {
	{
		AMD_PSTATES_MODULE_NAME,
		0,
		std_ops,
	},

	1.0f,

	pstates_set_scheduler_mode,

	pstates_increase_performance,
	pstates_decrease_performance,
};


module_info* modules[] = {
	(module_info*)&sAMDPStates,
	NULL
};

