/*
 * Copyright 2002-2018 Haiku, Inc. All rights reserved.
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

#if defined(B_HAIKU_64_BIT) && !defined(ELF32_COMPAT)
#	define _ELF_TYPE(type)	Elf64_##type
#else
#	define _ELF_TYPE(type)	Elf32_##type
#endif
#define DEFINE_ELF_TYPE(type, name) \
	typedef _ELF_TYPE(type) name

DEFINE_ELF_TYPE(Addr, elf_addr);
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
DEFINE_ELF_TYPE(Nhdr, elf_nhdr);
DEFINE_ELF_TYPE(Note_Team, elf_note_team);
DEFINE_ELF_TYPE(Note_Area_Entry, elf_note_area_entry);
DEFINE_ELF_TYPE(Note_Image_Entry, elf_note_image_entry);
DEFINE_ELF_TYPE(Note_Thread_Entry, elf_note_thread_entry);

#undef DEFINE_ELF_TYPE
#undef _ELF_TYPE

typedef uint16 elf_versym;

#if defined(B_HAIKU_64_BIT) && !defined(ELF32_COMPAT)
#	define ELF_CLASS	ELFCLASS64
#else
#	define ELF_CLASS	ELFCLASS32
#endif


#endif	/* _ELF_PRIVATE_H_ */
