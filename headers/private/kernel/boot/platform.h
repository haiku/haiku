/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_PLATFORM_H
#define KERNEL_BOOT_PLATFORM_H


#include <SupportDefs.h>
#include <list.h>


struct stage2_args;

#ifdef __cplusplus
extern "C" {
#endif

extern void panic(const char *format, ...);
extern status_t platform_get_boot_devices(struct stage2_args *args, struct list *devicesList);
extern void platform_release_heap(void *base);
extern status_t platform_init_heap(struct stage2_args *args, void **_base, void **_top);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_PLATFORM_H */
