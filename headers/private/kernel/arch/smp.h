/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_SMP_H
#define KERNEL_ARCH_SMP_H


#include <kernel.h>


struct kernel_args;

class CPUSet;


status_t arch_smp_init(kernel_args* args);
status_t arch_smp_per_cpu_init(kernel_args* args, int32 cpu);

void arch_smp_send_ici(int32 target_cpu);
void arch_smp_send_broadcast_ici();
void arch_smp_send_multicast_ici(CPUSet& cpuSet);


#endif	/* KERNEL_ARCH_SMP_H */
