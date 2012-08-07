/*
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H


#include <elf_common.h>
#include <thread.h>
#include <image.h>


struct kernel_args;


struct elf_symbol_info {
	addr_t	address;
	size_t	size;
};


#ifdef __cplusplus
extern "C" {
#endif

status_t elf_load_user_image(const char *path, Team *team, int flags,
	addr_t *_entry);

// these two might get public one day:
image_id load_kernel_add_on(const char *path);
status_t unload_kernel_add_on(image_id id);

status_t elf_debug_lookup_symbol_address(addr_t address, addr_t *_baseAddress,
			const char **_symbolName, const char **_imageName,
			bool *_exactMatch);
status_t elf_debug_lookup_user_symbol_address(Team* team, addr_t address,
			addr_t *_baseAddress, const char **_symbolName,
			const char **_imageName, bool *_exactMatch);
addr_t elf_debug_lookup_symbol(const char* searchName);
status_t elf_lookup_kernel_symbol(const char* name, elf_symbol_info* info);
struct elf_image_info* elf_get_kernel_image();
status_t elf_get_image_info_for_address(addr_t address, image_info* info);
image_id elf_create_memory_image(const char* imageName, addr_t text,
			size_t textSize, addr_t data, size_t dataSize);
status_t elf_add_memory_image_symbol(image_id id, const char* name,
			addr_t address, size_t size, int32 type);
status_t elf_init(struct kernel_args *args);

status_t _user_read_kernel_image_symbols(image_id id, elf_sym* symbolTable,
			int32* _symbolCount, char* stringTable, size_t* _stringTableSize,
			addr_t* _imageDelta);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ELF_H */
