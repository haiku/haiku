/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_H
#define KERNEL_BOOT_PLATFORM_H


#include <SupportDefs.h>
#include <boot/vfs.h>


struct stage2_args;

#ifdef __cplusplus
extern "C" {
#endif

/* debug functions */
extern void panic(const char *format, ...);
extern void dprintf(const char *format, ...);

/* heap functions */
extern void platform_release_heap(struct stage2_args *args, void *base);
extern status_t platform_init_heap(struct stage2_args *args, void **_base, void **_top);

/* MMU/memory functions */
extern status_t platform_allocate_region(void **_virtualAddress, size_t size,
	uint8 protection, bool exactAddress);
extern status_t platform_free_region(void *address, size_t size);
extern status_t platform_bootloader_address_to_kernel_address(void *address, uint64_t *_result);
extern status_t platform_kernel_address_to_bootloader_address(uint64_t address, void **_result);

/* boot options */
#define BOOT_OPTION_MENU			1
#define BOOT_OPTION_DEBUG_OUTPUT	2

extern uint32 platform_boot_options(void);

/* misc functions */
extern status_t platform_init_video(void);
extern void platform_switch_to_logo(void);
extern void platform_switch_to_text_mode(void);
extern void platform_start_kernel(void);
extern void platform_exit(void);

#ifdef __cplusplus
}

// these functions have to be implemented in C++

/* device functions */

class Node;
namespace boot {
	class Partition;
}

extern status_t platform_add_boot_device(struct stage2_args *args, NodeList *devicesList);
extern status_t platform_add_block_devices(struct stage2_args *args, NodeList *devicesList);
extern status_t platform_get_boot_partitions(struct stage2_args *args, Node *bootDevice,
					NodeList *partitions, NodeList *bootPartitions);
extern status_t platform_register_boot_device(Node *device);

/* menu functions */

class Menu;
class MenuItem;

extern void platform_add_menus(Menu *menu);
extern void platform_update_menu_item(Menu *menu, MenuItem *item);
extern void platform_run_menu(Menu *menu);
extern size_t platform_get_user_input_text(Menu *menu, MenuItem *item,
	char *buffer, size_t bufferSize);
extern char* platform_debug_get_log_buffer(size_t* _size);

#endif

#endif	/* KERNEL_BOOT_PLATFORM_H */
