/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CPU_H
#define CPU_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void arch_spin(bigtime_t microseconds);
extern status_t boot_arch_cpu_init(void);
extern void cpu_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* CPU_H */
