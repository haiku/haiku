/*
 * Copyright 2002-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYMBOLPATCHER_ELF_H
#define SYMBOLPATCHER_ELF_H


#include <elf.h>

/* Relocation types. */
#define R_X86_64_NONE				0	/* No relocation. */
#define R_X86_64_GLOB_DAT			6	/* Create GOT entry. */
#define R_X86_64_JUMP_SLOT			7	/* Create PLT entry. */

#define R_386_NONE		0
#define R_386_GLOB_DAT	6	/* set GOT entry to data address */
#define R_386_JMP_SLOT	7	/* set GOT entry to code address */

#ifdef B_HAIKU_64_BIT
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym Elf_Sym;
typedef Elf64_Rel Elf_Rel;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Word Elf_Word;
#define ELFCLASS ELFCLASS64
#define ELF_ST_BIND ELF64_ST_BIND
#define ELF_ST_TYPE ELF64_ST_TYPE
#define ELF_R_SYM ELF64_R_SYM
#define ELF_R_TYPE ELF64_R_TYPE
#define R_NONE R_X86_64_NONE
#define R_GLOB_DAT R_X86_64_GLOB_DAT
#define R_JUMP_SLOT R_X86_64_JUMP_SLOT
#else
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym Elf_Sym;
typedef Elf32_Rel Elf_Rel;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Word Elf_Word;
#define ELFCLASS ELFCLASS32
#define ELF_ST_BIND ELF32_ST_BIND
#define ELF_ST_TYPE ELF32_ST_TYPE
#define ELF_R_SYM ELF32_R_SYM
#define ELF_R_TYPE ELF32_R_TYPE
#define R_NONE R_386_NONE
#define R_GLOB_DAT R_386_GLOB_DAT
#define R_JUMP_SLOT R_386_JMP_SLOT
#endif


#endif	/* SYMBOLPATCHER_ELF_H */
