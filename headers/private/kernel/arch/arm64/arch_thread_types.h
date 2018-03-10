/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_
#define _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_


#include <kernel.h>


struct arch_thread {
	uint64			x[31];
	uint64			pc;
	uint64			sp;
	uint64			tpidr_el0;
	uint64			tpidrro_el0;
	int				last_vfp_cpu;
};

struct arch_team {
	int			dummy;
};

struct arch_fork_arg {
	int			dummy;
};


#endif /* _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_ */
