/* 
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <boot/stage2.h>
#include <arch/smp.h>
#include <debug.h>
#include <int.h>


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
#if KDEBUG
	if (are_interrupts_enabled())
		panic("arch_smp_send_multicast_ici: called with interrupts enabled");
#endif
}


void
arch_smp_send_ici(int32 target_cpu)
{
	panic("called arch_smp_send_ici!\n");
}


void
arch_smp_send_broadcast_ici()
{
	panic("called arch_smp_send_broadcast_ici\n");
}


