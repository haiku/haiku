/*
 * Copyright 2009 Haiku Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_MIPSEL_CPU_H
#define _KERNEL_ARCH_MIPSEL_CPU_H


#warning IMPLEMENT arch_cpu.h


#include <arch/mipsel/arch_thread_types.h>
#include <kernel.h>


struct iframe {
#warning struct iframe
	uint32 r31;
	uint32 r30;
	uint32 r29;
	uint32 r28;
	uint32 r27;
	uint32 r26;
	uint32 r25;
	uint32 r24;
	uint32 r23;
	uint32 r22;
	uint32 r21;
	uint32 r20;
	uint32 r19;
	uint32 r18;
	uint32 r17;
	uint32 r16;
	uint32 r15;
	uint32 r14;
	uint32 r13;
	uint32 r12;
	uint32 r11;
	uint32 r10;
	uint32 r9;
	uint32 r8;
	uint32 r7;
	uint32 r6;
	uint32 r5;
	uint32 r4;
	uint32 r3;
	uint32 r2;
	uint32 r1;
	uint32 r0;
};

typedef struct arch_cpu_info {
	int null;
} arch_cpu_info;


#ifdef __cplusplus
extern "C" {
#endif


extern long long get_time_base(void);

void __mipsel_setup_system_time(vint32 *cvFactor);
	// defined in libroot: os/arch/system_time.c

int64 __mipsel_get_time_base(void);
	// defined in libroot: os/arch/system_time_asm.S

extern void mipsel_context_switch(void **_oldStackPointer,
	void *newStackPointer);

extern bool mipsel_set_fault_handler(addr_t *handlerLocation, addr_t handler)
	__attribute__((noinline));

#ifdef __cplusplus
}
#endif


/* TODO */
enum mipsel_processor_version {
	FAKE1		= 0x0001,
	FAKE2		= 0x0002,
	FAKE3		= 0x0003,
};


#endif	/* _KERNEL_ARCH_MIPSEL_CPU_H */

