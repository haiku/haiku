/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_BOOT_ITEM_H
#define _KERNEL_BOOT_ITEM_H

#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

extern status_t boot_item_init(void);

extern status_t add_boot_item(const char *name, void *data, size_t size);
extern void *get_boot_item(const char *name, size_t *_size);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_BOOT_ITEM_H */
