/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
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


// #pragma mark - runtime loader libroot interface


struct user_space_program_args;
struct SymbolLookupInfo;

struct rld_export {
	// runtime loader API export
	image_id (*load_add_on)(char const *path, uint32 flags);
	status_t (*unload_add_on)(image_id imageID);
	image_id (*load_library)(char const *path, uint32 flags, void **_handle);
	status_t (*unload_library)(void* handle);
	status_t (*get_image_symbol)(image_id imageID, char const *symbolName,
		int32 symbolType, bool recursive, image_id *_inImage, void **_location);
	status_t (*get_library_symbol)(void* handle, void* caller,
		const char* symbolName, void **_location);
	status_t (*get_nth_image_symbol)(image_id imageID, int32 num,
		char *symbolName, int32 *nameLength, int32 *symbolType,
		void **_location);
	status_t (*get_nearest_symbol_at_address)(void* address,
		image_id* _imageID,	char** _imagePath, char** _symbolName,
		int32* _type, void** _location);
	status_t (*test_executable)(const char *path, char *interpreter);
	status_t (*get_next_image_dependency)(image_id id, uint32 *cookie,
		const char **_name);

	status_t (*reinit_after_fork)();

	void (*call_atexit_hooks_for_range)(addr_t start, addr_t size);

	void (*call_termination_hooks)();

	const struct user_space_program_args *program_args;
};

extern struct rld_export *__gRuntimeLoader;


// #pragma mark - runtime loader debugger interface


struct RuntimeLoaderSymbolPatcher;

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

typedef struct elf_version_info {
	uint32		hash;			// version name hash
	const char	*name;			// version name
	const char	*file_name;		// dependency file name (needed versions only)
} elf_version_info;

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

	uint32				api_version;
	uint32				abi;

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

	// versioning related structures
	uint32				num_version_definitions;
	struct Elf32_Verdef	*version_definitions;
	uint32				num_needed_versions;
	struct Elf32_Verneed *needed_versions;
	Elf32_Versym		*symbol_versions;
	elf_version_info	*versions;
	uint32				num_versions;

#ifdef __cplusplus
	struct Elf32_Sym*	(*find_undefined_symbol)(struct image_t* rootImage,
							struct image_t* image,
							const SymbolLookupInfo& lookupInfo,
							struct image_t** foundInImage);
#else
	struct Elf32_Sym*	(*find_undefined_symbol)(struct image_t* rootImage,
							struct image_t* image,
							const struct SymbolLookupInfo* lookupInfo,
							struct image_t** foundInImage);
#endif

	// Singly-linked list of symbol patchers for symbols defined respectively
	// referenced by this image.
	struct RuntimeLoaderSymbolPatcher	*defined_symbol_patchers;
	struct RuntimeLoaderSymbolPatcher	*undefined_symbol_patchers;

	// describes the text and data regions
	uint32				num_regions;
	elf_region_t		regions[1];
} image_t;

typedef struct image_queue_t {
	image_t *head;
	image_t *tail;
} image_queue_t;

// image_t::flags
#define	IMAGE_FLAG_RTLD_MASK			0x03
			// RTLD_{LAZY,NOW} | RTLD_{LOCAL,GLOBAL}

#define STRING(image, offset) ((char*)(&(image)->strtab[(offset)]))
#define SYMNAME(image, sym) STRING(image, (sym)->st_name)
#define SYMBOL(image, num) (&(image)->syms[num])
#define HASHTABSIZE(image) ((image)->symhash[0])
#define HASHBUCKETS(image) ((unsigned int*)&(image)->symhash[2])
#define HASHCHAINS(image) ((unsigned int*)&(image)->symhash[2+HASHTABSIZE(image)])


// The name of the area the runtime loader creates for debugging purposes.
#define RUNTIME_LOADER_DEBUG_AREA_NAME	"_rld_debug_"

// The contents of the runtime loader debug area.
typedef struct runtime_loader_debug_area {
	image_queue_t	*loaded_images;
} runtime_loader_debug_area;


// #pragma mark - runtime loader add-on interface


// symbol patcher callback
typedef void runtime_loader_symbol_patcher(void* cookie,
	struct image_t* rootImage, struct image_t* image, const char* name,
	struct image_t** foundInImage, void** symbol, int32* type);

// interface provided to add-ons
struct runtime_loader_add_on_export {
	status_t	(*register_defined_symbol_patcher)(struct image_t* image,
					runtime_loader_symbol_patcher* patcher, void* cookie);
	void		(*unregister_defined_symbol_patcher)(struct image_t* image,
					runtime_loader_symbol_patcher* patcher, void* cookie);
	status_t	(*register_undefined_symbol_patcher)(struct image_t* image,
					runtime_loader_symbol_patcher* patcher, void* cookie);
	void		(*unregister_undefined_symbol_patcher)(struct image_t* image,
					runtime_loader_symbol_patcher* patcher, void* cookie);
};


#define RUNTIME_LOADER_ADD_ON_VERSION	1

typedef struct runtime_loader_add_on {
	uint32	version;
	uint32	flags;

	// called after the add-on image has been loaded
	void	(*init)(struct rld_export* standardInterface,
				struct runtime_loader_add_on_export* addOnInterface);

	// called whenever the respective image event occurs
	void	(*image_loaded)(struct image_t* image);
	void	(*image_relocated)(struct image_t* image);
	void	(*image_initialized)(struct image_t* image);
	void	(*image_uninitializing)(struct image_t* image);
	void	(*image_unloading)(struct image_t* image);
} runtime_loader_add_on;

// This is the variable a preloaded shared object has to export to get picked up
// by the runtime loader as an add-on.
extern runtime_loader_add_on __gRuntimeLoaderAddOn;

#endif	// _RUNTIME_LOADER_H
