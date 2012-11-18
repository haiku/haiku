/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_ELF_H
#define _KERNEL_ARCH_ELF_H


#include <elf_common.h>


struct elf_image_info;


#ifdef __cplusplus
extern "C" {
#endif

extern int arch_elf_relocate_rel(struct elf_image_info *image,
	struct elf_image_info *resolve_image, elf_rel *rel, int rel_len);
extern int arch_elf_relocate_rela(struct elf_image_info *image,
	struct elf_image_info *resolve_image, elf_rela *rel, int rel_len);

#ifdef __cplusplus
}
#endif


#include <arch_elf.h>


#endif	/* _KERNEL_ARCH_ELF_H */
