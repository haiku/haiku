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

int elf_load_uspace(const char *path, struct team *t, int flags, addr *entry);
image_id elf_load_kspace(const char *path, const char *sym_prepend);
int elf_unload_kspace(const char *path);
addr elf_lookup_symbol(image_id id, const char *symbol);
int elf_lookup_symbol_address(addr address, addr *baseAddress, char *text, size_t length);
int elf_init(kernel_args *ka);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ELF_H */
