/*
 * Copyright 2004-2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#ifdef _BOOT_MODE
#	include <boot/arch.h>
#endif

#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>


//#define TRACE_ARCH_ELF
#ifdef TRACE_ARCH_ELF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#ifndef _BOOT_MODE
static bool
is_in_image(struct elf_image_info *image, addr_t address)
{
	return (address >= image->text_region.start
			&& address < image->text_region.start + image->text_region.size)
		|| (address >= image->data_region.start
			&& address < image->data_region.start + image->data_region.size);
}
#endif	// !_BOOT_MODE


#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rel(preloaded_elf64_image* image, Elf64_Rel* rel,
	int relLength)
#else
int
arch_elf_relocate_rel(struct elf_image_info *image,
	struct elf_image_info *resolveImage, Elf64_Rel *rel, int relLength)
#endif
{
	dprintf("arch_elf_relocate_rel: not supported on arm64\n");
	return B_ERROR;
}


#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rela(preloaded_elf64_image* image, Elf64_Rela* rel,
	int relLength)
#else
int
arch_elf_relocate_rela(struct elf_image_info *image,
	struct elf_image_info *resolveImage, Elf64_Rela *rel, int relLength)
#endif
{
	for (int i = 0; i < relLength / (int)sizeof(Elf64_Rela); i++) {
		int type = ELF64_R_TYPE(rel[i].r_info);
		int symIndex = ELF64_R_SYM(rel[i].r_info);
		Elf64_Addr symAddr = 0;

		// Resolve the symbol, if any.
		if (symIndex != 0) {
			Elf64_Sym* symbol = SYMBOL(image, symIndex);

			status_t status;
#ifdef _BOOT_MODE
			status = boot_elf_resolve_symbol(image, symbol, &symAddr);
#else
			status = elf_resolve_symbol(image, symbol, resolveImage, &symAddr);
#endif
			if (status < B_OK)
				return status;
		}

		// Address of the relocation.
		Elf64_Addr relocAddr = image->text_region.delta + rel[i].r_offset;

		// Calculate the relocation value.
		Elf64_Addr relocValue;
		switch (type) {
			default:
				dprintf("arch_elf_relocate_rela: unhandled relocation type %d\n",
					type);
				return B_BAD_DATA;
		}
#ifdef _BOOT_MODE
		boot_elf64_set_relocation(relocAddr, relocValue);
#else
		if (!is_in_image(image, relocAddr)) {
			dprintf("arch_elf_relocate_rela: invalid offset %#lx\n",
				rel[i].r_offset);
			return B_BAD_ADDRESS;
		}
#endif
	}

	return B_OK;
}
