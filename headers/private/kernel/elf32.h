/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _ELF32_H
#define _ELF32_H


#include <SupportDefs.h>
#include <ByteOrder.h>

#include <arch/elf.h>


typedef uint32 Elf32_Addr;
typedef uint16 Elf32_Half;
typedef uint32 Elf32_Off;
typedef int32 Elf32_Sword;
typedef uint32 Elf32_Word;


/*** ELF header ***/

#define EI_NIDENT	16

struct Elf32_Ehdr {
	uint8		e_ident[EI_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;

#ifdef __cplusplus
	bool IsHostEndian() const;
#endif
};

#define ELF_MAGIC	"\x7f""ELF"

// e_ident[] indices
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

// architecture class (EI_CLASS)
#define ELFCLASS32	1
#define ELFCLASS64	2
// endian (EI_DATA)
#define ELFDATA2LSB	1	// little endian
#define ELFDATA2MSB	2	// big endian


/*** section header ***/

struct Elf32_Shdr {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
};

// special section indices
#define SHN_UNDEF		0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC		0xff00
#define SHN_HIPROC		0xff1f
#define SHN_ABS			0xfff1
#define SHN_COMMON		0xfff2
#define SHN_HIRESERVE	0xffff

// section header type
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

// section header flags
#define SHF_WRITE		1
#define SHF_ALLOC		2
#define SHF_EXECINSTR	4

#define SHF_MASKPROC	0xf0000000


/*** program header ***/

struct Elf32_Phdr {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;	/* offset from the beginning of the file of the segment */
	Elf32_Addr	p_vaddr;	/* virtual address for the segment in memory */
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;	/* the size of the segment in the file */
	Elf32_Word	p_memsz;	/* the size of the segment in memory */
	Elf32_Word	p_flags;
	Elf32_Word	p_align;

#ifdef __cplusplus
	bool IsReadWrite() const;
	bool IsExecutable() const;
#endif
};

// program header segment types
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6

#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

// program header segment flags
#define PF_EXECUTE	0x1
#define PF_WRITE	0x2
#define PF_READ		0x4
#define PF_PROTECTION_MASK (PF_EXECUTE | PF_WRITE | PF_READ)

#define PF_MASKPROC	0xf0000000

struct Elf32_Sym {
	Elf32_Word	st_name;
	Elf32_Addr	st_value;
	Elf32_Word	st_size;
	uint8		st_info;
	uint8 		st_other;
	Elf32_Half	st_shndx;

#ifdef __cplusplus
	uint8 Bind() const;
	uint8 Type() const;
#endif
};

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STN_UNDEF 0

struct Elf32_Rel {
	Elf32_Addr r_offset;
	Elf32_Word r_info;

#ifdef __cplusplus
	uint8 SymbolIndex() const;
	uint8 Type() const;
#endif
};

#ifdef __cplusplus
struct Elf32_Rela : public Elf32_Rel {
#else
struct Elf32_Rela {
	Elf32_Addr r_offset;
	Elf32_Word r_info;
#endif
	Elf32_Sword r_addend;
};

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (unsigned char)(t))

struct Elf32_Dyn {
	Elf32_Sword d_tag;
	union {
		Elf32_Word d_val;
		Elf32_Addr d_ptr;
	} d_un;
};

#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23

#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7fffffff


/*** inline functions ***/

#ifdef __cplusplus

inline bool 
Elf32_Ehdr::IsHostEndian() const
{
#if B_HOST_IS_LENDIAN
	return e_ident[EI_DATA] == ELFDATA2LSB;
#elif B_HOST_IS_BENDIAN
	return e_ident[EI_DATA] == ELFDATA2MSB;
#endif
}


inline bool 
Elf32_Phdr::IsReadWrite() const
{
	return !(~p_flags & (PF_READ | PF_WRITE));
}


inline bool
Elf32_Phdr::IsExecutable() const
{
	return (p_flags & PF_PROTECTION_MASK) == (PF_READ | PF_EXECUTE);
}


inline uint8 
Elf32_Sym::Bind() const
{
	return ELF32_ST_BIND(st_info);
}


inline uint8 
Elf32_Sym::Type() const
{
	return ELF32_ST_TYPE(st_info);
}


inline uint8 
Elf32_Rel::SymbolIndex() const
{
	return ELF32_R_SYM(r_info);
}


inline uint8 
Elf32_Rel::Type() const
{
	return ELF32_R_TYPE(r_info);
}

#endif	/* __cplusplus */

#endif	/* _ELF32_H_ */
