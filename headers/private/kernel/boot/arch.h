/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_ARCH_H
#define KERNEL_BOOT_ARCH_H

#include <SupportDefs.h>
#include <boot/elf.h>


#ifdef __cplusplus
extern "C" {
#endif

/* ELF support */

extern status_t boot_arch_elf_relocate_rel(preloaded_elf32_image *image,
	struct Elf32_Rel *rel, int rel_len);
extern status_t boot_arch_elf_relocate_rela(preloaded_elf32_image *image,
	struct Elf32_Rela *rel, int rel_len);


#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_ARCH_H */
