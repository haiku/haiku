/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_
#define _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_


#include <kernel.h>


struct arch_thread {
	uint64 regs[13]; // x19-x30, sp
	uint64 fp_regs[8]; // d8-d15
};

struct arch_team {
	int			dummy;
};

struct arch_fork_arg {
	int			dummy;
};

#endif /* _KERNEL_ARCH_ARM64_ARCH_THREAD_TYPES_H_ */
