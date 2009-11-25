/*
 * Copyright 2002-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ELF_H
#define _ELF_H


#include <SupportDefs.h>


// types
typedef uint32		Elf32_Addr;
typedef uint32		Elf32_Off;
typedef uint16		Elf32_Half;
typedef int32		Elf32_Sword;
typedef uint32		Elf32_Word;
typedef uint32		Elf32_Xword;
typedef int32		Elf32_Sxword;

typedef uint64		Elf64_Addr;
typedef uint64		Elf64_Off;
typedef uint16		Elf64_Half;
typedef int32		Elf64_Sword;
typedef uint32		Elf64_Word;
typedef uint64		Elf64_Xword;
typedef int64		Elf64_Sxword;

// e_ident indices
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7
#define EI_NIDENT	16

// e_ident EI_VERSION values
#define EV_NONE 	0
#define EV_CURRENT 	1

// e_ident EI_CLASS and EI_DATA values
#define ELFCLASSNONE	0
#define ELFCLASS32		1
#define ELFCLASS64		2
#define ELFDATANONE		0
#define ELFDATA2LSB		1
#define ELFDATA2MSB		2

// p_type
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDIR	6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

// sh_type values
#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

// 32 bit definitions
#undef _ELFX_BITS
#define _ELFX_BITS	32
#include <ElfX.h>

// 64 bit definitions
#undef _ELFX_BITS
#define _ELFX_BITS	64
#include <ElfX.h>

#undef _ELFX_BITS


#endif	// _ELF_H
