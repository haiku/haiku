/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_SMP_H
#define KERNEL_SMP_H


#include <KernelExport.h>

struct kernel_args;


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

#ifdef __cplusplus
extern "C" {
#endif

status_t smp_init(struct kernel_args *args);
status_t smp_per_cpu_init(struct kernel_args *args, int32 cpu);
bool smp_trap_non_boot_cpus(int32 cpu);
void smp_wake_up_non_boot_cpus(void);
void smp_wait_for_non_boot_cpus(void);
void smp_send_ici(int32 targetCPU, int32 message, uint32 data, uint32 data2, uint32 data3,
		void *data_ptr, uint32 flags);
void smp_send_broadcast_ici(int32 message, uint32 data, uint32 data2, uint32 data3,
		void *data_ptr, uint32 flags);

int32 smp_get_num_cpus(void);
void smp_set_num_cpus(int32 numCPUs);
int32 smp_get_current_cpu(void);

int smp_intercpu_int_handler(void);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_SMP_H */
