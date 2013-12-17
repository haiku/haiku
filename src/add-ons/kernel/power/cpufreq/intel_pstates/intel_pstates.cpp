/*
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


#define INTEL_PSTATES_MODULE_NAME	CPUFREQ_MODULES_PREFIX "/intel_pstates/v1"


const int kMinimalInterval = 50000;

static uint16 sMinPState;
static uint16 sMaxPState;
static uint16 sBoostPState;

static bool sAvoidBoost;


struct CPUEntry {
				CPUEntry();

	uint16		fCurrentPState;

	bigtime_t	fLastUpdate;
} CACHE_LINE_ALIGN;
static CPUEntry* sCPUEntries;


CPUEntry::CPUEntry()
	:
	fCurrentPState(sMinPState - 1),
	fLastUpdate(0)
{
}


static void
pstates_set_scheduler_mode(scheduler_mode mode)
{
	sAvoidBoost = mode == SCHEDULER_MODE_POWER_SAVING;
}


static int
measure_pstate(CPUEntry* entry)
{
	InterruptsLocker locker;

	uint64 mperf = x86_read_msr(IA32_MSR_MPERF);
	uint64 aperf = x86_read_msr(IA32_MSR_APERF);

	x86_write_msr(IA32_MSR_MPERF, 0);
	x86_write_msr(IA32_MSR_APERF, 0);

	locker.Unlock();

	if (mperf == 0)
		return sMinPState;

	int oldPState = sMaxPState * aperf / mperf;
	oldPState = min_c(max_c(oldPState, sMinPState), sBoostPState);

	return oldPState;
}


static inline void
set_pstate(uint16 pstate)
{
	CPUEntry* entry = &sCPUEntries[smp_get_current_cpu()];
	pstate = min_c(max_c(sMinPState, pstate), sBoostPState);

	if (entry->fCurrentPState != pstate) {
		entry->fLastUpdate = system_time();
		entry->fCurrentPState = pstate;

		x86_write_msr(IA32_MSR_PERF_CTL, pstate << 8);
	}
}


static status_t
pstates_increase_performance(int delta)
{
	CPUEntry* entry = &sCPUEntries[smp_get_current_cpu()];

	if (system_time() - entry->fLastUpdate < kMinimalInterval)
		return B_OK;

	int pState = measure_pstate(entry);
	pState += (sBoostPState - pState) * delta / kCPUPerformanceScaleMax;

	if (sAvoidBoost && pState < (sMaxPState + sBoostPState) / 2)
		pState = min_c(pState, sMaxPState);

	set_pstate(pState);
	return B_OK;
}


static status_t
pstates_decrease_performance(int delta)
{
	CPUEntry* entry = &sCPUEntries[smp_get_current_cpu()];

	if (system_time() - entry->fLastUpdate < kMinimalInterval)
		return B_OK;

	int pState = measure_pstate(entry);
	pState -= (pState - sMinPState) * delta / kCPUPerformanceScaleMax;

	set_pstate(pState);
	return B_OK;
}


static bool
is_cpu_model_supported(cpu_ent* cpu)
{
	uint8 model = cpu->arch.model + (cpu->arch.extended_model << 4);

	if (cpu->arch.vendor != VENDOR_INTEL)
		return false;

	if (cpu->arch.family != 6)
		return false;

	const uint8 kSupportedFamily6Models[] = {
		0x2a, 0x2d, 0x2e, 0x3a, 0x3c, 0x3e, 0x3f, 0x45, 0x46,
	};
	const int kSupportedFamily6ModelsCount
		= sizeof(kSupportedFamily6Models) / sizeof(uint8);

	int i;
	for (i = 0; i < kSupportedFamily6ModelsCount; i++) {
		if (model == kSupportedFamily6Models[i])
			break;
	}

	return i != kSupportedFamily6ModelsCount;
}


static void
set_normal_pstate(void* /* dummy */, int cpu)
{
	measure_pstate(&sCPUEntries[cpu]);
	set_pstate(sMaxPState);
}


static status_t
init_pstates()
{
	if (!x86_check_feature(IA32_FEATURE_MSR, FEATURE_COMMON))
		return B_ERROR;

	if (!x86_check_feature(IA32_FEATURE_APERFMPERF, FEATURE_6_ECX))
		return B_ERROR;

	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++) {
		if (!is_cpu_model_supported(&gCPU[i]))
			return B_ERROR;
	}

	sMinPState = (x86_read_msr(IA32_MSR_PLATFORM_INFO) >> 40) & 0xff;
	sMaxPState = (x86_read_msr(IA32_MSR_PLATFORM_INFO) >> 8) & 0xff;
	sBoostPState
		= max_c(x86_read_msr(IA32_MSR_TURBO_RATIO_LIMIT) & 0xff, sMaxPState);

	dprintf("using Intel P-States: min %" B_PRIu16 ", max %" B_PRIu16
		", boost %" B_PRIu16 "\n", sMinPState, sMaxPState, sBoostPState);

	if (sMaxPState <= sMinPState || sMaxPState == 0) {
		dprintf("unexpected or invalid Intel P-States limits, aborting\n");
		return B_ERROR;
	}

	sCPUEntries = new(std::nothrow) CPUEntry[cpuCount];
	if (sCPUEntries == NULL)
		return B_NO_MEMORY;

	pstates_set_scheduler_mode(SCHEDULER_MODE_LOW_LATENCY);

	call_all_cpus_sync(set_normal_pstate, NULL);
	return B_OK;
}


static status_t
uninit_pstates()
{
	call_all_cpus_sync(set_normal_pstate, NULL);
	delete[] sCPUEntries;

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


static cpufreq_module_info sIntelPStates = {
	{
		INTEL_PSTATES_MODULE_NAME,
		0,
		std_ops,
	},

	1.0f,

	pstates_set_scheduler_mode,

	pstates_increase_performance,
	pstates_decrease_performance,
};


module_info* modules[] = {
	(module_info*)&sIntelPStates,
	NULL
};

