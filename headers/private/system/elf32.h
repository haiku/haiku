/*
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 *
 * Copyright 2002-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _ELF32_H
#define _ELF32_H


#include <elf_common.h>


typedef uint32 Elf32_Addr;
typedef uint16 Elf32_Half;
typedef uint32 Elf32_Off;
typedef int32 Elf32_Sword;
typedef uint32 Elf32_Word;

typedef Elf32_Half Elf32_Versym;

/*** ELF header ***/

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


/* auxiliary needed version information */

struct Elf32_Vernaux {
	Elf32_Word	vna_hash;		/* dependency name hash value */
	Elf32_Half	vna_flags;		/* dependency specific information flags */
	Elf32_Half	vna_other;		/* version index as specified in the symbol
								   version table */
	Elf32_Word	vna_name;		/* string table offset to dependency name */
	Elf32_Word	vna_next;		/* byte offset to next vernaux entry */
};


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
