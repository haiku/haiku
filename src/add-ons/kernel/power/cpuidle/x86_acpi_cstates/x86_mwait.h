/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifdef __cplusplus
extern "C" {
#endif


#define MWAIT_INTERRUPTS_BREAK		(1 << 0)


static inline void
x86_monitor(void* address, uint32 ecx, uint32 edx)
{
	asm volatile("monitor" : : "a" (address), "c" (ecx), "d"(edx));
}


static inline void
x86_mwait(uint32 eax, uint32 ecx)
{
	asm volatile("mwait" : : "a" (eax), "c" (ecx));
}


#ifdef __cplusplus
}
#endif
