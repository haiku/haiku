/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_SYMBOL_LOOKUP_H
#define ELF_SYMBOL_LOOKUP_H

#include <runtime_loader.h>


// values for SymbolLookupInfo::flags
#define LOOKUP_FLAG_DEFAULT_VERSION	0x01


uint32 elf_hash(const char* name);


struct SymbolLookupInfo {
	const char*				name;
	int32					type;
	uint32					hash;
	uint32					flags;
	const elf_version_info*	version;

	SymbolLookupInfo(const char* name, int32 type, uint32 hash,
		const elf_version_info* version = NULL, uint32 flags = 0)
		:
		name(name),
		type(type),
		hash(hash),
		flags(flags),
		version(version)
	{
	}

	SymbolLookupInfo(const char* name, int32 type,
		const elf_version_info* version = NULL, uint32 flags = 0)
		:
		name(name),
		type(type),
		hash(elf_hash(name)),
		flags(flags),
		version(version)
	{
	}
};


void		patch_defined_symbol(image_t* image, const char* name,
				void** symbol, int32* type);
void		patch_undefined_symbol(image_t* rootImage, image_t* image,
				const char* name, image_t** foundInImage, void** symbol,
				int32* type);

Elf32_Sym*	find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo);
status_t	find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo,
				void** _location);
status_t	find_symbol_breadth_first(image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** _foundInImage,
				void** _location);
Elf32_Sym*	find_undefined_symbol_beos(image_t* rootImage, image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** foundInImage);
Elf32_Sym*	find_undefined_symbol_global(image_t* rootImage, image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** foundInImage);
Elf32_Sym*	find_undefined_symbol_add_on(image_t* rootImage, image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** foundInImage);


#endif	// ELF_SYMBOL_LOOKUP_H
