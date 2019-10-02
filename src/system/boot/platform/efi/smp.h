/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SMP_H
#define SMP_H


#include <SupportDefs.h>


#ifdef __cplusplus
// this is only available in C++
#	include <boot/menu.h>
extern void smp_add_safemode_menus(Menu *menu);

extern "C" {
#endif

extern void smp_init(void);
extern void smp_init_other_cpus(void);
extern void smp_boot_other_cpus(uint32 pml4, uint64 kernel_entry);
extern int smp_get_current_cpu(void);

#ifdef __cplusplus
}
#endif


#endif	/* SMP_H */
