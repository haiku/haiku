/*
 * Copyright 2002-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ELF_PRIV_H
#define _KERNEL_ELF_PRIV_H


#include <elf32.h>
#include <elf64.h>
#include <image.h>


struct elf_version_info;


typedef struct elf_region {
	area_id		id;
	addr_t		start;
	addr_t		size;
	long		delta;
} elf_region;

struct elf_image_info {
	struct elf_image_info *next;	// next image in the hash
	char		*name;
	image_id	id;
	int32		ref_count;
	struct vnode *vnode;
	elf_region	text_region;
	elf_region	data_region;
	addr_t		dynamic_section;		// pointer to the dynamic section
	struct elf_linked_image *linked_images;

	bool		symbolic;

	struct Elf32_Ehdr *elf_header;

	// pointer to symbol participation data structures
	char		*needed;
	uint32		*symhash;
	struct Elf32_Sym *syms;
	char		*strtab;
	struct Elf32_Rel *rel;
	int			rel_len;
	struct Elf32_Rela *rela;
	int			rela_len;
	struct Elf32_Rel *pltrel;
	int			pltrel_len;
	int			pltrel_type;

	struct Elf32_Sym *debug_symbols;
	uint32		num_debug_symbols;
	const char	*debug_string_table;

	// versioning related structures
	uint32				num_version_definitions;
	struct Elf32_Verdef	*version_definitions;
	uint32				num_needed_versions;
	struct Elf32_Verneed *needed_versions;
	Elf32_Versym		*symbol_versions;
	struct elf_version_info	*versions;
	uint32				num_versions;
};


#define STRING(image, offset) ((char *)(&(image)->strtab[(offset)]))
#define SYMNAME(image, sym) STRING(image, (sym)->st_name)
#define SYMBOL(image, num) ((struct Elf32_Sym *)&(image)->syms[num])
#define HASHTABSIZE(image) ((image)->symhash[0])
#define HASHBUCKETS(image) ((unsigned int *)&(image)->symhash[2])
#define HASHCHAINS(image) ((unsigned int *)&(image)->symhash[2+HASHTABSIZE(image)])

extern
#ifdef __cplusplus
"C"
#endif
status_t elf_resolve_symbol(struct elf_image_info *image, struct Elf32_Sym *sym,
			struct elf_image_info *shared_image, addr_t *sym_addr);

#endif	/* _KERNEL_ELF_PRIV_H */
