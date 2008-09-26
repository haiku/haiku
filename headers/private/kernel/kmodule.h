/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H


#include <drivers/module.h>
#include <kernel.h>

struct kernel_args;


#ifdef __cplusplus
// C++ only part

class NotificationListener;

extern status_t start_watching_modules(const char *prefix,
	NotificationListener &listener);
extern status_t stop_watching_modules(const char *prefix,
	NotificationListener &listener);


extern "C" {
#endif

extern status_t unload_module(const char *path);
extern status_t load_module(const char *path, module_info ***_modules);

extern status_t module_init(struct kernel_args *args);
extern status_t module_init_post_threads(void);
extern status_t module_init_post_boot_device(bool bootingFromBootLoaderVolume);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_MODULE_H */
