/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>

#define ELF_TRACE 0
#if ELF_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


int
arch_elf_relocate_rel(struct elf_image_info *image, const char *sym_prepend,
	struct elf_image_info *resolve_image, struct Elf32_Rel *rel, int rel_len)
{
	dprintf("arch_elf_relocate_rel: not supported on PPC\n");
	return B_ERROR;
}


int
arch_elf_relocate_rela(struct elf_image_info *image, const char *sym_prepend,
	struct elf_image_info *resolve_image, struct Elf32_Rela *rel, int rel_len)
{
	dprintf("arch_elf_relocate_rela: not supported on PPC\n");
	return B_ERROR;
}

