/*
 * Copyright 2002-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001 Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _ELF_PRIVATE_H
#define _ELF_PRIVATE_H


#include <os/kernel/elf.h>

#include <SupportDefs.h>

#include <arch_elf.h>


// Determine the correct ELF types to use for the architecture

#if B_HAIKU_64_BIT
#	define _ELF_TYPE(type)	Elf64_##type
#else
#	define _ELF_TYPE(type)	Elf32_##type
#endif
#define DEFINE_ELF_TYPE(type, name) \
	typedef _ELF_TYPE(type) name

DEFINE_ELF_TYPE(Ehdr, elf_ehdr);
DEFINE_ELF_TYPE(Phdr, elf_phdr);
DEFINE_ELF_TYPE(Shdr, elf_shdr);
DEFINE_ELF_TYPE(Sym, elf_sym);
DEFINE_ELF_TYPE(Dyn, elf_dyn);
DEFINE_ELF_TYPE(Rel, elf_rel);
DEFINE_ELF_TYPE(Rela, elf_rela);
DEFINE_ELF_TYPE(Verdef, elf_verdef);
DEFINE_ELF_TYPE(Verdaux, elf_verdaux);
DEFINE_ELF_TYPE(Verneed, elf_verneed);
DEFINE_ELF_TYPE(Vernaux, elf_vernaux);

#undef DEFINE_ELF_TYPE
#undef _ELF_TYPE

typedef uint16 elf_versym;

#if B_HAIKU_64_BIT
#	define ELF_CLASS	ELFCLASS64
#else
#	define ELF_CLASS	ELFCLASS32
#endif


#endif	/* _ELF_PRIVATE_H_ */
