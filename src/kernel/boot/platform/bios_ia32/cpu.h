/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef CPU_H
#define CPU_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern void cpu_init(void);
extern void cpu_boot_other_cpus(void);

#ifdef __cplusplus
}
#endif

#endif	/* CPU_H */
