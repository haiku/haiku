/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_PLATFORM_H
#define KERNEL_BOOT_PLATFORM_H


#include <SupportDefs.h>
#include <util/list.h>


struct stage2_args;

#ifdef __cplusplus
extern "C" {
#endif

/* debug functions */
extern void panic(const char *format, ...);
extern void dprintf(const char *format, ...);

/* heap functions */
extern void platform_release_heap(void *base);
extern status_t platform_init_heap(struct stage2_args *args, void **_base, void **_top);

/* misc functions */
extern bool platform_user_menu_requested(void);

#ifdef __cplusplus
}

class Node;
namespace boot {
	class Partition;
}

/* device functions */

// these functions have to be implemented in C++
extern status_t platform_get_boot_device(struct stage2_args *args, Node **_device);
extern status_t platform_add_block_devices(struct stage2_args *args, struct list *devicesList);
extern status_t platform_get_boot_partition(struct stage2_args *args, struct list *partitions,
					boot::Partition **_partition);

#endif

#endif	/* KERNEL_BOOT_PLATFORM_H */
