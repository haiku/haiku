/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <KernelExport.h>

#include <boot/stage2.h>
#include <arch/smp.h>
#include <debug.h>
#include <interrupts.h>
#include <smp.h>

#include "soc.h"


status_t
arch_smp_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_smp_per_cpu_init(kernel_args *args, int32 cpu)
{
	return B_OK;
}


void
arch_smp_send_multicast_ici(CPUSet& cpuSet)
{
	InterruptController *ic = InterruptController::Get();
	ic->SendMulticastIci(cpuSet);
}


void
arch_smp_send_ici(int32 target_cpu)
{
	InterruptController *ic = InterruptController::Get();
	CPUSet cpuSet;
	cpuSet.SetBit(target_cpu);
	ic->SendMulticastIci(cpuSet);
}


void
arch_smp_send_broadcast_ici()
{
	InterruptController *ic = InterruptController::Get();
	ic->SendBroadcastIci();
}
