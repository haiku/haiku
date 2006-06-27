/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#ifndef _RUNTIME_LOADER_H
#define _RUNTIME_LOADER_H

#include <image.h>
#include <OS.h>

#include <elf32.h>

typedef struct elf_region_t {
	area_id		id;
	addr_t		start;
	addr_t		size;
	addr_t		vmstart;
	addr_t		vmsize;
	addr_t		fdstart;
	addr_t		fdsize;
	long		delta;
	uint32		flags;
} elf_region_t;

typedef struct image_t {
	// image identification
	char				path[B_PATH_NAME_LENGTH];
	char				name[B_OS_NAME_LENGTH];
	image_id			id;
	image_type			type;

	struct image_t		*next;
	struct image_t		*prev;
	int32				ref_count;
	uint32				flags;

	addr_t 				entry_point;
	addr_t				init_routine;
	addr_t				term_routine;
	addr_t 				dynamic_ptr; 	// pointer to the dynamic section

	// pointer to symbol participation data structures
	uint32				*symhash;
	struct Elf32_Sym	*syms;
	char				*strtab;
	struct Elf32_Rel	*rel;
	int					rel_len;
	struct Elf32_Rela	*rela;
	int					rela_len;
	struct Elf32_Rel	*pltrel;
	int					pltrel_len;

	uint32				num_needed;
	struct image_t		**needed;

	// describes the text and data regions
	uint32				num_regions;
	elf_region_t		regions[1];
} image_t;

typedef struct image_queue_t {
	image_t *head;
	image_t *tail;
} image_queue_t;

#define STRING(image, offset) ((char *)(&(image)->strtab[(offset)]))
#define SYMNAME(image, sym) STRING(image, (sym)->st_name)
#define SYMBOL(image, num) ((struct Elf32_Sym *)&(image)->syms[num])
#define HASHTABSIZE(image) ((image)->symhash[0])
#define HASHBUCKETS(image) ((unsigned int *)&(image)->symhash[2])
#define HASHCHAINS(image) ((unsigned int *)&(image)->symhash[2+HASHTABSIZE(image)])


// The name of the area the runtime loader creates for debugging purposes.
#define RUNTIME_LOADER_DEBUG_AREA_NAME	"_rld_debug_"

// The contents of the runtime loader debug area.
typedef struct runtime_loader_debug_area {
	image_queue_t	*loaded_images;
} runtime_loader_debug_area;

#endif	// _RUNTIME_LOADER_H
