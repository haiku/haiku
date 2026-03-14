/*
 * Copyright 2026 John Davis
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <arch_cpu.h>

#include "hyperv_cpu.h"
#include "VMBusDevicePrivate.h"


status_t
VMBusDevice::GetReferenceCounter(uint64* _counter)
{
	if (!fReferenceCounterSupported)
		return B_NOT_SUPPORTED;

	*_counter = x86_read_msr(IA32_MSR_HV_TIME_REF_COUNT);
	return B_OK;
}


bool
VMBusDevice::_IsReferenceCounterSupported()
{
	cpuid_info cpuInfo;
	get_cpuid(&cpuInfo, IA32_CPUID_LEAF_HV_FEAT_ID, 0);
	return (cpuInfo.regs.eax & HV_CPUID_FEATURE_TIME_REF_COUNTER) != 0;
}
