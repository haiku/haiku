/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DAEMON_H
#define _KERNEL_DAEMON_H


#include <KernelExport.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t register_resource_resizer(daemon_hook function, void* arg,
			int frequency);
status_t unregister_resource_resizer(daemon_hook function, void* arg);

status_t kernel_daemon_init(void);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_DAEMON_H */
