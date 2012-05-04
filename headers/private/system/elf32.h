/*
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 *
 * Copyright 2002-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _ELF32_H
#define _ELF32_H


#include <SupportDefs.h>
#include <ByteOrder.h>

#include <arch_elf.h>


typedef uint32 Elf32_Addr;
typedef uint16 Elf32_Half;
typedef uint32 Elf32_Off;
typedef int32 Elf32_Sword;
typedef uint32 Elf32_Word;

typedef Elf32_Half Elf32_Versym;

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

// e_machine (Architecture)
#define EM_NONE			0 // No machine
#define EM_M32			1 // AT&T WE 32100
#define EM_SPARC		2 // Sparc
#define EM_386			3 // Intel 80386
#define EM_68K			4 // Motorola m68k family
#define EM_88K			5 // Motorola m88k family
#define EM_486			6 // Intel 80486, Reserved for future use
#define EM_860			7 // Intel 80860
#define EM_MIPS			8 // MIPS R3000 (officially, big-endian only)
#define EM_S370			9 // IBM System/370
#define EM_MIPS_RS3_LE	10 // MIPS R3000 little-endian, Deprecated
#define EM_PARISC		15 // HPPA
#define EM_VPP550		17 // Fujitsu VPP500
#define EM_SPARC32PLUS	18 // Sun "v8plus"
#define EM_960			19 // Intel 80960
#define EM_PPC			20 // PowerPC
#define EM_PPC64		21 // 64-bit PowerPC
#define EM_S390			22 // IBM S/390
#define EM_V800			36 // NEC V800 series
#define EM_FR20			37 // Fujitsu FR20
#define EM_RH32			38 // TRW RH32
#define EM_MCORE		39 // Motorola M*Core
#define EM_RCE			39 // Old name for MCore
#define EM_ARM			40 // ARM
#define EM_OLD_ALPHA	41 // Digital Alpha
#define EM_SH			42 // Renesas / SuperH SH
#define EM_SPARCV9		43 // SPARC v9 64-bit
#define EM_TRICORE		44 // Siemens Tricore embedded processor
#define EM_ARC			45 // ARC Cores
#define EM_H8_300		46 // Renesas H8/300
#define EM_H8_300H		47 // Renesas H8/300H
#define EM_H8S			48 // Renesas H8S
#define EM_H8_500		49 // Renesas H8/500
#define EM_IA_64		50 // Intel IA-64 Processor
#define EM_MIPS_X		51 // Stanford MIPS-X
#define EM_COLDFIRE		52 // Motorola Coldfire
#define EM_68HC12		53 // Motorola M68HC12
#define EM_MMA			54 // Fujitsu Multimedia Accelerator
#define EM_PCP			55 // Siemens PCP
#define EM_NCPU			56 // Sony nCPU embedded RISC processor
#define EM_NDR1			57 // Denso NDR1 microprocesspr
#define EM_STARCORE		58 // Motorola Star*Core processor
#define EM_ME16			59 // Toyota ME16 processor
#define EM_ST100		60 // STMicroelectronics ST100 processor
#define EM_TINYJ		61 // Advanced Logic Corp. TinyJ embedded processor
#define EM_X86_64		62 // Advanced Micro Devices X86-64 processor

// architecture class (EI_CLASS)
#define ELFCLASS32	1
#define ELFCLASS64	2
// endian (EI_DATA)
#define ELFDATA2LSB	1	/* little endian */
#define ELFDATA2MSB	2	/* big endian */


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

#define SHT_GNU_verdef	0x6ffffffd    /* version definition section */
#define SHT_GNU_verneed	0x6ffffffe    /* version needs section */
#define SHT_GNU_versym	0x6fffffff    /* version symbol table */

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
#define PT_STACK	0x6474e551

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

#define DT_NULL				0
#define DT_NEEDED			1
#define DT_PLTRELSZ			2
#define DT_PLTGOT			3
#define DT_HASH				4
#define DT_STRTAB			5
#define DT_SYMTAB			6
#define DT_RELA				7
#define DT_RELASZ			8
#define DT_RELAENT			9
#define DT_STRSZ			10
#define DT_SYMENT			11
#define DT_INIT				12
#define DT_FINI				13
#define DT_SONAME			14
#define DT_RPATH			15
#define DT_SYMBOLIC			16
#define DT_REL				17
#define DT_RELSZ			18
#define DT_RELENT			19
#define DT_PLTREL			20
#define DT_DEBUG			21
#define DT_TEXTREL			22
#define DT_JMPREL			23
#define DT_BIND_NOW			24	/* no lazy binding */
#define DT_INIT_ARRAY		25	/* init function array */
#define DT_FINI_ARRAY		26	/* termination function array */
#define DT_INIT_ARRAYSZ		27	/* init function array size */
#define DT_FINI_ARRAYSZ		28	/* termination function array size */
#define DT_RUNPATH			29	/* library search path (supersedes DT_RPATH) */
#define DT_FLAGS			30	/* flags (see below) */
#define DT_ENCODING			32
#define DT_PREINIT_ARRAY	32	/* preinitialization array */
#define DT_PREINIT_ARRAYSZ	33	/* preinitialization array size */

#define DT_VERSYM       0x6ffffff0	/* symbol version table */
#define DT_VERDEF		0x6ffffffc	/* version definition table */
#define DT_VERDEFNUM	0x6ffffffd	/* number of version definitions */
#define DT_VERNEED		0x6ffffffe 	/* table with needed versions */
#define DT_VERNEEDNUM	0x6fffffff	/* number of needed versions */

#define DT_LOPROC		0x70000000
#define DT_HIPROC		0x7fffffff


/* DT_FLAGS values */
#define DF_ORIGIN		0x01
#define DF_SYMBOLIC		0x02
#define DF_TEXTREL		0x04
#define DF_BIND_NOW		0x08
#define DF_STATIC_TLS	0x10


/* version definition section */

struct Elf32_Verdef {
	Elf32_Half	vd_version;		/* version revision */
	Elf32_Half	vd_flags;		/* version information flags */
	Elf32_Half	vd_ndx;			/* version index as specified in the
								   symbol version table */
	Elf32_Half	vd_cnt;			/* number of associated verdaux entries */
	Elf32_Word	vd_hash;		/* version name hash value */
	Elf32_Word	vd_aux;			/* byte offset to verdaux array */
	Elf32_Word	vd_next;		/* byte offset to next verdef entry */
};

/* values for vd_version (version revision) */
#define VER_DEF_NONE		0		/* no version */
#define VER_DEF_CURRENT		1		/* current version */
#define VER_DEF_NUM			2		/* given version number */

/* values for vd_flags (version information flags) */
#define VER_FLG_BASE		0x1		/* version definition of file itself */
#define VER_FLG_WEAK		0x2 	/* weak version identifier */

/* values for versym symbol index */
#define VER_NDX_LOCAL		0		/* symbol is local */
#define VER_NDX_GLOBAL		1		/* symbol is global/unversioned */
#define VER_NDX_INITIAL		2		/* initial version -- that's the one given
									   to symbols when a library becomes
									   versioned; handled by the linker (and
									   runtime loader) similar to
									   VER_NDX_GLOBAL */
#define VER_NDX_LORESERVE	0xff00	/* beginning of reserved entries */
#define VER_NDX_ELIMINATE	0xff01	/* symbol is to be eliminated */

#define VER_NDX_FLAG_HIDDEN	0x8000	/* flag: version is hidden */
#define VER_NDX_MASK		0x7fff	/* mask to get the actual version index */
#define VER_NDX(x)			((x) & VER_NDX_MASK)


/* auxiliary version information */

struct Elf32_Verdaux {
	Elf32_Word	vda_name;		/* string table offset to version or dependency
								   name */
	Elf32_Word	vda_next;		/* byte offset to next verdaux entry */
};


/* version dependency section */

struct Elf32_Verneed {
	Elf32_Half	vn_version;		/* version of structure */
	Elf32_Half	vn_cnt;			/* number of associated vernaux entries */
	Elf32_Word	vn_file;		/* byte offset to file name for this
								   dependency */
	Elf32_Word	vn_aux;			/* byte offset to vernaux array */
	Elf32_Word	vn_next;		/* byte offset to next verneed entry */
};

/* values for vn_version (version revision) */
#define VER_NEED_NONE		0	/* no version */
#define VER_NEED_CURRENT	1	/* current version */
#define VER_NEED_NUM		2	/* given version number */


/* auxiliary needed version information */

struct Elf32_Vernaux {
	Elf32_Word	vna_hash;		/* dependency name hash value */
	Elf32_Half	vna_flags;		/* dependency specific information flags */
	Elf32_Half	vna_other;		/* version index as specified in the symbol
								   version table */
	Elf32_Word	vna_name;		/* string table offset to dependency name */
	Elf32_Word	vna_next;		/* byte offset to next vernaux entry */
};

/* values for vna_flags */
#define VER_FLG_WEAK	0x2		/* weak version identifier */


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
	return (p_flags & PF_EXECUTE) != 0;
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
