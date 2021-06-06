/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_RISCV64_CPU_H
#define _KERNEL_ARCH_RISCV64_CPU_H


#include <arch/riscv64/arch_thread_types.h>
#include <arch_cpu_defs.h>
#include <kernel.h>


#define CPU_MAX_CACHE_LEVEL	8
#define CACHE_LINE_SIZE		64


static inline bool
get_ac()
{
	SstatusReg status(Sstatus());
	return status.sum != 0;
}


static inline void
set_ac()
{
	// TODO: Could be done atomically via CSRRS?
	SstatusReg status(Sstatus());
	status.sum = 1;
	SetSstatus(status.val);
}


static inline void
clear_ac()
{
	// TODO: Could be done atomically with CSRRC?
	SstatusReg status(Sstatus());
	status.sum = 0;
	SetSstatus(status.val);
}


typedef struct arch_cpu_info {
	int null;
} arch_cpu_info;


#ifdef __cplusplus
extern "C" {
#endif


void __riscv64_setup_system_time(uint64 conversionFactor);


static inline void
arch_cpu_pause(void)
{
	// TODO: CPU pause
}


static inline void
arch_cpu_idle(void)
{
	Wfi();
}


#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_ARCH_RISCV64_CPU_H */
