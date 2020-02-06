/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_SYMBOL_LOOKUP_H
#define ELF_SYMBOL_LOOKUP_H


#include <stdlib.h>
#include <string.h>

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
	elf_sym*				requestingSymbol;

	SymbolLookupInfo(const char* name, int32 type, uint32 hash,
		const elf_version_info* version = NULL, uint32 flags = 0,
		elf_sym* requestingSymbol = NULL)
		:
		name(name),
		type(type),
		hash(hash),
		flags(flags),
		version(version),
		requestingSymbol(requestingSymbol)
	{
	}

	SymbolLookupInfo(const char* name, int32 type,
		const elf_version_info* version = NULL, uint32 flags = 0,
		elf_sym* requestingSymbol = NULL)
		:
		name(name),
		type(type),
		hash(elf_hash(name)),
		flags(flags),
		version(version),
		requestingSymbol(requestingSymbol)
	{
	}
};


struct SymbolLookupCache {
	SymbolLookupCache(image_t* image)
		:
		fTableSize(image->symhash != NULL ? image->symhash[1] : 0),
		fValues(NULL),
		fDSOs(NULL),
		fValuesResolved(NULL)
	{
		if (fTableSize > 0) {
			fValues = (addr_t*)malloc(sizeof(addr_t) * fTableSize);
			fDSOs = (image_t**)malloc(sizeof(image_t*) * fTableSize);

			size_t elementCount = (fTableSize + 31) / 32;
			fValuesResolved = (uint32*)malloc(4 * elementCount);

			if (fValues == NULL || fDSOs == NULL || fValuesResolved == NULL) {
				free(fValuesResolved);
				fValuesResolved = NULL;
				free(fValues);
				fValues = NULL;
				free(fDSOs);
				fDSOs = NULL;
				fTableSize = 0;
			} else {
				memset(fValuesResolved, 0, 4 * elementCount);
			}
		}
	}

	~SymbolLookupCache()
	{
		free(fValuesResolved);
		free(fValues);
		free(fDSOs);
	}

	bool IsSymbolValueCached(size_t index) const
	{
		return index < fTableSize
			&& (fValuesResolved[index / 32] & (1 << (index % 32))) != 0;
	}

	addr_t SymbolValueAt(size_t index) const
	{
		return fValues[index];
	}

	addr_t SymbolValueAt(size_t index, image_t** image) const
	{
		if (image)
			*image = fDSOs[index];
		return fValues[index];
	}

	void SetSymbolValueAt(size_t index, addr_t value, image_t* image)
	{
		if (index < fTableSize) {
			fValues[index] = value;
			fDSOs[index] = image;
			fValuesResolved[index / 32] |= 1 << (index % 32);
		}
	}

private:
	size_t		fTableSize;
	addr_t*		fValues;
	image_t**	fDSOs;
	uint32*		fValuesResolved;
};


void		patch_defined_symbol(image_t* image, const char* name,
				void** symbol, int32* type);
void		patch_undefined_symbol(image_t* rootImage, image_t* image,
				const char* name, image_t** foundInImage, void** symbol,
				int32* type);

elf_sym*	find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo,
				bool allowLocal = false);
status_t	find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo,
				void** _location);
status_t	find_symbol_breadth_first(image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** _foundInImage,
				void** _location);
elf_sym*	find_undefined_symbol_beos(image_t* rootImage, image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** foundInImage);
elf_sym*	find_undefined_symbol_global(image_t* rootImage, image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** foundInImage);
elf_sym*	find_undefined_symbol_add_on(image_t* rootImage, image_t* image,
				const SymbolLookupInfo& lookupInfo, image_t** foundInImage);


#endif	// ELF_SYMBOL_LOOKUP_H
