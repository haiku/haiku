/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_SMP_H
#define _KERNEL_SMP_H

#include <stage2.h>

// intercpu messages
enum {
	SMP_MSG_INVL_PAGE_RANGE = 0,
	SMP_MSG_INVL_PAGE_LIST,
	SMP_MSG_GLOBAL_INVL_PAGE,
	SMP_MSG_RESCHEDULE,
	SMP_MSG_CPU_HALT,
	SMP_MSG_1,
};

enum {
	SMP_MSG_FLAG_ASYNC = 0,
	SMP_MSG_FLAG_SYNC,
};

int smp_init(kernel_args *ka);
int smp_trap_non_boot_cpus(kernel_args *ka, int cpu);
void smp_wake_up_all_non_boot_cpus(void);
void smp_wait_for_ap_cpus(kernel_args *ka);
void smp_send_ici(int target_cpu, int message, unsigned long data, unsigned long data2, unsigned long data3, void *data_ptr, int flags);
void smp_send_broadcast_ici(int message, unsigned long data, unsigned long data2, unsigned long data3, void *data_ptr, int flags);
int smp_enable_ici(void);
int smp_disable_ici(void);

int smp_get_num_cpus(void);
void smp_set_num_cpus(int num_cpus);
int smp_get_current_cpu(void);

// spinlock functions
typedef volatile int spinlock_t;
void acquire_spinlock(spinlock_t *lock);
void release_spinlock(spinlock_t *lock);

#endif

