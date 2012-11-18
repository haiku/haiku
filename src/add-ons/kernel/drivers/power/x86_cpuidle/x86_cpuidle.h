/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifdef __cplusplus
extern "C" {
#endif

extern CpuidleModuleInfo *gIdle;

static inline void
x86_monitor(const void *addr, unsigned long ecx, unsigned long edx)
{
	asm volatile("monitor"
		:: "a" (addr), "c" (ecx), "d"(edx));
}

static inline void
x86_mwait(unsigned long eax, unsigned long ecx)
{
	asm volatile("mwait"
		:: "a" (eax), "c" (ecx));
}

status_t intel_cpuidle_init(void);

#ifdef __cplusplus
}
#endif
