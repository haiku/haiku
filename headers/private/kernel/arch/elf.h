/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_ELF_H
#define _KERNEL_ARCH_ELF_H

struct Elf32_Rel;
struct Elf32_Rela;
struct elf_image_info;

#ifdef __cplusplus
extern "C" {
#endif

extern int arch_elf_relocate_rel(struct elf_image_info *image,
	struct elf_image_info *resolve_image, struct Elf32_Rel *rel, int rel_len);
extern int arch_elf_relocate_rela(struct elf_image_info *image,
	struct elf_image_info *resolve_image, struct Elf32_Rela *rel, int rel_len);

#ifdef __cplusplus
}
#endif

#include <arch_elf.h>

#endif	/* _KERNEL_ARCH_ELF_H */
