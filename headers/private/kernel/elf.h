/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H


#include <thread.h>
#include <image.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t elf_load_user_image(const char *path, struct team *team, int flags, addr_t *_entry);

// these two might get public one day:
image_id load_kernel_add_on(const char *path);
status_t unload_kernel_add_on(image_id id);

status_t elf_lookup_symbol_address(addr_t address, addr_t *baseAddress, char *text, size_t length);
status_t elf_init(kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ELF_H */
