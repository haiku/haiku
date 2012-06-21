/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_ELF_H
#define KERNEL_BOOT_ELF_H


#include <boot/addr_range.h>
#include <sys/stat.h>
#include <elf_priv.h>
#include <util/FixedWidthPointer.h>


struct preloaded_image {
	FixedWidthPointer<struct preloaded_image> next;
	char		*name;
	elf_region	text_region;
	elf_region	data_region;
	addr_range	dynamic_section;
	struct Elf32_Ehdr elf_header;

	struct Elf32_Sym	*syms;
	struct Elf32_Rel	*rel;
	int					rel_len;
	struct Elf32_Rela	*rela;
	int					rela_len;
	struct Elf32_Rel	*pltrel;
	int					pltrel_len;
	int					pltrel_type;

	struct Elf32_Sym *debug_symbols;
	const char	*debug_string_table;
	uint32		num_debug_symbols, debug_string_table_size;

	ino_t		inode;
	image_id	id;
		// the ID field will be filled out in the kernel
	bool		is_module;
		// set by the module initialization code
} _PACKED;

#ifdef __cplusplus
extern "C" {
#endif

extern status_t boot_elf_resolve_symbol(struct preloaded_image *image,
	struct Elf32_Sym *symbol, addr_t *symbolAddress);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_ELF_H */
