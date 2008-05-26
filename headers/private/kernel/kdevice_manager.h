/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEVICE_MANAGER_H
#define _KERNEL_DEVICE_MANAGER_H


#include <device_manager.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

void legacy_driver_add_preloaded(struct kernel_args *args);

status_t device_manager_probe(const char *path, uint32 updateCycle);
status_t device_manager_init(struct kernel_args *args);
status_t device_manager_init_post_modules(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_DEVICE_MANAGER_H */
