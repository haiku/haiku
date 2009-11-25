/*
 * Copyright 2002-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

// No header guard: The file is included twice by <Elf.h> and must not be
// included elsewhere. The _ELFX_BITS macro must be define before inclusion.


#undef ElfX

#if _ELFX_BITS == 32
#	define ElfX(x)	Elf32_##x
#elif _ELFX_BITS == 64
#	define ElfX(x)	Elf64_##x
#endif


// object file header
typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	ElfX(Half)		e_type;
	ElfX(Half)		e_machine;
	ElfX(Word)		e_version;
	ElfX(Addr)		e_entry;
	ElfX(Off)		e_phoff;
	ElfX(Off)		e_shoff;
	ElfX(Word)		e_flags;
	ElfX(Half)		e_ehsize;
	ElfX(Half)		e_phentsize;
	ElfX(Half)		e_phnum;
	ElfX(Half)		e_shentsize;
	ElfX(Half)		e_shnum;
	ElfX(Half)		e_shstrndx;
} ElfX(Ehdr);

// program header
typedef struct {
	ElfX(Word)	p_type;
#if _ELFX_BITS == 64
	ElfX(Word)	p_flags;
#endif
	ElfX(Off)	p_offset;
	ElfX(Addr)	p_vaddr;
	ElfX(Addr)	p_paddr;
	ElfX(Xword)	p_filesz;
	ElfX(Xword)	p_memsz;
#if _ELFX_BITS == 32
	ElfX(Word)	p_flags;
#endif
	ElfX(Xword)	p_align;
} ElfX(Phdr);

// section header
typedef struct {
	ElfX(Word)	sh_name;
	ElfX(Word)	sh_type;
	ElfX(Xword)	sh_flags;
	ElfX(Addr)	sh_addr;
	ElfX(Off)	sh_offset;
	ElfX(Xword)	sh_size;
	ElfX(Word)	sh_link;
	ElfX(Word)	sh_info;
	ElfX(Xword)	sh_addralign;
	ElfX(Xword)	sh_entsize;
} ElfX(Shdr);

#undef ElfX
