/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_SYSTEM_INFO_H
#define _KERNEL_ARCH_SYSTEM_INFO_H


#include <OS.h>
#include <arch_system_info.h>


struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_system_info_init(struct kernel_args *args);
status_t arch_get_system_info(system_info *info, size_t size);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_ARCH_SYSTEM_INFO_H */
