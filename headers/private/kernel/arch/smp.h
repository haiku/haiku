/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_SMP_H
#define _NEWOS_KERNEL_ARCH_SMP_H

#include <kernel.h>
#include <stage2.h>

// must match MAX_BOOT_CPUS in stage2.h
#define SMP_MAX_CPUS MAX_BOOT_CPUS

int arch_smp_init(kernel_args *ka);
void arch_smp_send_ici(int target_cpu);
void arch_smp_send_broadcast_ici(void);

#endif

