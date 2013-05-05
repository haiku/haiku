/*
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 *
 * Copyright 2002-2011, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _ELF64_H
#define _ELF64_H


#include <elf_common.h>


typedef uint64 Elf64_Addr;
typedef uint64 Elf64_Off;
typedef uint16 Elf64_Half;
typedef uint32 Elf64_Word;
typedef int32 Elf64_Sword;
typedef uint64 Elf64_Xword;
typedef int64 Elf64_Sxword;

typedef Elf64_Half Elf64_Versym;

/*** ELF header ***/

struct Elf64_Ehdr {
	uint8		e_ident[EI_NIDENT];
	Elf64_Half	e_type;
	Elf64_Half	e_machine;
	Elf64_Word	e_version;
	Elf64_Addr	e_entry;
	Elf64_Off	e_phoff;
	Elf64_Off	e_shoff;
	Elf64_Word	e_flags;
	Elf64_Half	e_ehsize;
	Elf64_Half	e_phentsize;
	Elf64_Half	e_phnum;
	Elf64_Half	e_shentsize;
	Elf64_Half	e_shnum;
	Elf64_Half	e_shstrndx;

#ifdef __cplusplus
	bool IsHostEndian() const;
#endif
};


/*** section header ***/

struct Elf64_Shdr {
	Elf64_Word	sh_name;
	Elf64_Word	sh_type;
	Elf64_Xword	sh_flags;
	Elf64_Addr	sh_addr;
	Elf64_Off	sh_offset;
	Elf64_Xword	sh_size;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Xword	sh_addralign;
	Elf64_Xword	sh_entsize;
};


/*** program header ***/

struct Elf64_Phdr {
	Elf64_Word	p_type;
	Elf64_Word	p_flags;
	Elf64_Off	p_offset;	/* offset from the beginning of the file of the segment */
	Elf64_Addr	p_vaddr;	/* virtual address for the segment in memory */
	Elf64_Addr	p_paddr;
	Elf64_Xword	p_filesz;	/* the size of the segment in the file */
	Elf64_Xword	p_memsz;	/* the size of the segment in memory */
	Elf64_Xword	p_align;

#ifdef __cplusplus
	bool IsReadWrite() const;
	bool IsExecutable() const;
#endif
};

struct Elf64_Sym {
	Elf64_Word	st_name;
	uint8		st_info;
	uint8		st_other;
	Elf64_Half	st_shndx;
	Elf64_Addr	st_value;
	Elf64_Xword	st_size;

#ifdef __cplusplus
	uint8 Bind() const;
	uint8 Type() const;
	void SetInfo(uint8 bind, uint8 type);
#endif
};

#define ELF64_ST_BIND(i) ((i) >> 4)
#define ELF64_ST_TYPE(i) ((i) & 0xf)
#define ELF64_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

struct Elf64_Rel {
	Elf64_Addr	r_offset;
	Elf64_Xword	r_info;

#ifdef __cplusplus
	uint8 SymbolIndex() const;
	uint8 Type() const;
#endif
};

#ifdef __cplusplus
struct Elf64_Rela : public Elf64_Rel {
#else
struct Elf64_Rela {
	Elf64_Addr		r_offset;
	Elf64_Xword		r_info;
#endif
	Elf64_Sxword	r_addend;
};

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)
#define ELF64_R_INFO(s, t) ((((Elf64_Xword)(s)) << 32) + ((t) & 0xffffffffL))

struct Elf64_Dyn {
	Elf64_Sxword d_tag;
	union {
		Elf64_Xword	d_val;
		Elf64_Addr	d_ptr;
	} d_un;
};


/* version definition section */

struct Elf64_Verdef {
	Elf64_Half	vd_version;		/* version revision */
	Elf64_Half	vd_flags;		/* version information flags */
	Elf64_Half	vd_ndx;			/* version index as specified in the
								   symbol version table */
	Elf64_Half	vd_cnt;			/* number of associated verdaux entries */
	Elf64_Word	vd_hash;		/* version name hash value */
	Elf64_Word	vd_aux;			/* byte offset to verdaux array */
	Elf64_Word	vd_next;		/* byte offset to next verdef entry */
};


/* auxiliary version information */

struct Elf64_Verdaux {
	Elf64_Word	vda_name;		/* string table offset to version or dependency
								   name */
	Elf64_Word	vda_next;		/* byte offset to next verdaux entry */
};


/* version dependency section */

struct Elf64_Verneed {
	Elf64_Half	vn_version;		/* version of structure */
	Elf64_Half	vn_cnt;			/* number of associated vernaux entries */
	Elf64_Word	vn_file;		/* byte offset to file name for this
								   dependency */
	Elf64_Word	vn_aux;			/* byte offset to vernaux array */
	Elf64_Word	vn_next;		/* byte offset to next verneed entry */
};


/* auxiliary needed version information */

struct Elf64_Vernaux {
	Elf64_Word	vna_hash;		/* dependency name hash value */
	Elf64_Half	vna_flags;		/* dependency specific information flags */
	Elf64_Half	vna_other;		/* version index as specified in the symbol
								   version table */
	Elf64_Word	vna_name;		/* string table offset to dependency name */
	Elf64_Word	vna_next;		/* byte offset to next vernaux entry */
};


/*** inline functions ***/

#ifdef __cplusplus

inline bool
Elf64_Ehdr::IsHostEndian() const
{
#if B_HOST_IS_LENDIAN
	return e_ident[EI_DATA] == ELFDATA2LSB;
#elif B_HOST_IS_BENDIAN
	return e_ident[EI_DATA] == ELFDATA2MSB;
#endif
}


inline bool
Elf64_Phdr::IsReadWrite() const
{
	return !(~p_flags & (PF_READ | PF_WRITE));
}


inline bool
Elf64_Phdr::IsExecutable() const
{
	return (p_flags & PF_EXECUTE) != 0;
}


inline uint8
Elf64_Sym::Bind() const
{
	return ELF64_ST_BIND(st_info);
}


inline uint8
Elf64_Sym::Type() const
{
	return ELF64_ST_TYPE(st_info);
}


inline void
Elf64_Sym::SetInfo(uint8 bind, uint8 type)
{
	st_info = ELF64_ST_INFO(bind, type);
}


inline uint8
Elf64_Rel::SymbolIndex() const
{
	return ELF64_R_SYM(r_info);
}


inline uint8
Elf64_Rel::Type() const
{
	return ELF64_R_TYPE(r_info);
}

#endif	/* __cplusplus */

#endif	/* _ELF64_H_ */
