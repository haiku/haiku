/*
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_SPARC_CPU_H
#define _KERNEL_ARCH_SPARC_CPU_H


#include <arch/sparc/arch_thread_types.h>
#include <arch/sparc/cpu.h>
#include <kernel.h>


#define CPU_MAX_CACHE_LEVEL	8
#define CACHE_LINE_SIZE		128
	// 128 Byte lines on PPC970


#define set_ac()
#define clear_ac()


typedef struct arch_cpu_info {
	int null;
} arch_cpu_info;


#ifdef __cplusplus
extern "C" {
#endif


static inline void
arch_cpu_pause(void)
{
	// TODO: CPU pause
}


static inline void
arch_cpu_idle(void)
{
	// TODO: CPU idle call
}


#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_SPARC_CPU_H */
