/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_ARCH_PPC_CPU_H
#define _KERNEL_ARCH_PPC_CPU_H

#include <arch/ppc/thread_struct.h>

#define PAGE_SIZE 4096

#define _BIG_ENDIAN 1
#define _LITTLE_ENDIAN 0

#define ATOMIC64_FUNCS_ARE_SYSCALLS 1

struct iframe {
};

#ifdef __cplusplus
extern "C" {
#endif

extern uint32 get_sdr1(void);
extern void set_sdr1(uint32 value);
extern uint32 get_sr(void *virtualAddress);
extern void set_sr(void *virtualAddress, uint32 value);
extern uint32 get_msr(void);
extern uint32 set_msr(uint32 value);

#ifdef __cplusplus
}
#endif

#define eieio()	asm volatile("eieio")

#endif	/* _KERNEL_ARCH_PPC_CPU_H */
