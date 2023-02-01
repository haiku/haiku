/*
 * Copyright 2005-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _KERNEL_ARCH_RISCV64_INT_H
#define _KERNEL_ARCH_RISCV64_INT_H

#include <SupportDefs.h>
#include <arch_cpu_defs.h>

#define NUM_IO_VECTORS	256


#ifdef __cplusplus

static inline void
arch_int_enable_interrupts_inline(void)
{
	SetBitsSstatus(SstatusReg{.ie = 1 << modeS}.val);
}


static inline int
arch_int_disable_interrupts_inline(void)
{
	SstatusReg oldStatus{.val = GetAndClearBitsSstatus(SstatusReg{.ie = 1 << modeS}.val)};
	return ((1 << modeS) & oldStatus.ie) != 0;
}


static inline void
arch_int_restore_interrupts_inline(int oldState)
{
	if (oldState)
		arch_int_enable_interrupts_inline();
}


static inline bool
arch_int_are_interrupts_enabled_inline(void)
{
	SstatusReg status{.val = Sstatus()};
	return ((1 << modeS) & status.ie) != 0;
}


// map the functions to the inline versions
#define arch_int_enable_interrupts()	arch_int_enable_interrupts_inline()
#define arch_int_disable_interrupts()	arch_int_disable_interrupts_inline()
#define arch_int_restore_interrupts(status)	\
	arch_int_restore_interrupts_inline(status)
#define arch_int_are_interrupts_enabled()	\
	arch_int_are_interrupts_enabled_inline()


enum {
	kMSyscallSwitchToSmode = 0,
	kMSyscallSetTimer      = 1,
};

extern "C" status_t MSyscall(uint64 op, ...);

#endif


#endif /* _KERNEL_ARCH_RISCV64_INT_H */
