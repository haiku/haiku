/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_MODULE_H
#define _KERNEL_MODULE_H


#include <drivers/module.h>
#include <kernel.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

extern status_t unload_module(const char *path);
extern status_t load_module(const char *path, module_info ***_modules);

extern status_t module_init(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KRENEL_MODULE_H */
