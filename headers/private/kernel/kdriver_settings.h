/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DRIVER_SETTINGS_H
#define _KERNEL_DRIVER_SETTINGS_H


#include <SupportDefs.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

status_t driver_settings_init(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_DRIVER_SETTINGS_H */
