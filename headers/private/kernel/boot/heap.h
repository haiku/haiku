/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_BOOT_HEAP_H
#define KERNEL_BOOT_HEAP_H

#include <SupportDefs.h>
#include <boot/stage2_args.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void heap_release(struct stage2_args *args);
extern void heap_print_statistics();
extern status_t heap_init(struct stage2_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_HEAP_H */
