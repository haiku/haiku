/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "elf_symbol_lookup.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "add_ons.h"
#include "errors.h"
#include "images.h"
#include "runtime_loader_private.h"


static bool
equals_image_name(image_t* image, const char* name)
{
	const char* lastSlash = strrchr(name, '/');
	return strcmp(image->name, lastSlash != NULL ? lastSlash + 1 : name) == 0;
}


/*!	This function is called when we run BeOS images on Haiku.
	It allows us to redirect functions to ensure compatibility.
*/
static const char*
beos_compatibility_map_symbol(const char* symbolName)
{
	struct symbol_mapping {
		const char* from;
		const char* to;
	};
	static const struct symbol_mapping kMappings[] = {
		// TODO: Improve this, and also use it for libnet.so compatibility!
		// Allow an image to provide a function that will be invoked for every
		// (transitively) depending image. The function can return a table to
		// remap symbols (probably better address to address). All the tables
		// for a single image would be combined into a hash table and an
		// undefined symbol patcher using this hash table would be added.
		{"fstat", "__be_fstat"},
		{"lstat", "__be_lstat"},
		{"stat", "__be_stat"},
	};
	const uint32 kMappingCount = sizeof(kMappings) / sizeof(kMappings[0]);

	for (uint32 i = 0; i < kMappingCount; i++) {
		if (!strcmp(symbolName, kMappings[i].from))
			return kMappings[i].to;
	}

	return symbolName;
}


// #pragma mark -


uint32
elf_hash(const char* _name)
{
	const uint8* name = (const uint8*)_name;

	uint32 hash = 0;
	uint32 temp;

	while (*name) {
		hash = (hash << 4) + *name++;
		if ((temp = hash & 0xf0000000)) {
			hash ^= temp >> 24;
		}
		hash &= ~temp;
	}
	return hash;
}


void
patch_defined_symbol(image_t* image, const char* name, void** symbol,
	int32* type)
{
	RuntimeLoaderSymbolPatcher* patcher = image->defined_symbol_patchers;
	while (patcher != NULL && *symbol != 0) {
		image_t* inImage = image;
		patcher->patcher(patcher->cookie, NULL, image, name, &inImage,
			symbol, type);
		patcher = patcher->next;
	}
}


void
patch_undefined_symbol(image_t* rootImage, image_t* image, const char* name,
	image_t** foundInImage, void** symbol, int32* type)
{
	if (*foundInImage != NULL)
		patch_defined_symbol(*foundInImage, name, symbol, type);

	RuntimeLoaderSymbolPatcher* patcher = image->undefined_symbol_patchers;
	while (patcher != NULL) {
		patcher->patcher(patcher->cookie, rootImage, image, name, foundInImage,
			symbol, type);
		patcher = patcher->next;
	}
}


Elf32_Sym*
find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo)
{
	if (image->dynamic_ptr == 0)
		return NULL;

	Elf32_Sym* versionedSymbol = NULL;
	uint32 versionedSymbolCount = 0;

	uint32 bucket = lookupInfo.hash % HASHTABSIZE(image);

	for (uint32 i = HASHBUCKETS(image)[bucket]; i != STN_UNDEF;
			i = HASHCHAINS(image)[i]) {
		Elf32_Sym* symbol = &image->syms[i];

		if (symbol->st_shndx != SHN_UNDEF
			&& ((ELF32_ST_BIND(symbol->st_info)== STB_GLOBAL)
				|| (ELF32_ST_BIND(symbol->st_info) == STB_WEAK))
			&& !strcmp(SYMNAME(image, symbol), lookupInfo.name)) {

			// check if the type matches
			uint32 type = ELF32_ST_TYPE(symbol->st_info);
			if ((lookupInfo.type == B_SYMBOL_TYPE_TEXT && type != STT_FUNC)
				|| (lookupInfo.type == B_SYMBOL_TYPE_DATA
					&& type != STT_OBJECT)) {
				continue;
			}

			// check the version

			// Handle the simple cases -- the image doesn't have version
			// information -- first.
			if (image->symbol_versions == NULL) {
				if (lookupInfo.version == NULL) {
					// No specific symbol version was requested either, so the
					// symbol is just fine.
					return symbol;
				}

				// A specific version is requested. If it's the dependency
				// referred to by the requested version, it's apparently an
				// older version of the dependency and we're not happy.
				if (equals_image_name(image, lookupInfo.version->file_name)) {
					// TODO: That should actually be kind of fatal!
					return NULL;
				}

				// This is some other image. We accept the symbol.
				return symbol;
			}

			// The image has version information. Let's see what we've got.
			uint32 versionID = image->symbol_versions[i];
			uint32 versionIndex = VER_NDX(versionID);
			elf_version_info& version = image->versions[versionIndex];

			// skip local versions
			if (versionIndex == VER_NDX_LOCAL)
				continue;

			if (lookupInfo.version != NULL) {
				// a specific version is requested

				// compare the versions
				if (version.hash == lookupInfo.version->hash
					&& strcmp(version.name, lookupInfo.version->name) == 0) {
					// versions match
					return symbol;
				}

				// The versions don't match. We're still fine with the
				// base version, if it is public and we're not looking for
				// the default version.
				if ((versionID & VER_NDX_FLAG_HIDDEN) == 0
					&& versionIndex == VER_NDX_GLOBAL
					&& (lookupInfo.flags & LOOKUP_FLAG_DEFAULT_VERSION)
						== 0) {
					// TODO: Revise the default version case! That's how
					// FreeBSD implements it, but glibc doesn't handle it
					// specially.
					return symbol;
				}
			} else {
				// No specific version requested, but the image has version
				// information. This can happen in either of these cases:
				//
				// * The dependent object was linked against an older version
				//   of the now versioned dependency.
				// * The symbol is looked up via find_image_symbol() or dlsym().
				//
				// In the first case we return the base version of the symbol
				// (VER_NDX_GLOBAL or VER_NDX_INITIAL), or, if that doesn't
				// exist, the unique, non-hidden versioned symbol.
				//
				// In the second case we want to return the public default
				// version of the symbol. The handling is pretty similar to the
				// first case, with the exception that we treat VER_NDX_INITIAL
				// as regular version.

				// VER_NDX_GLOBAL is always good, VER_NDX_INITIAL is fine, if
				// we don't look for the default version.
				if (versionIndex == VER_NDX_GLOBAL
					|| ((lookupInfo.flags & LOOKUP_FLAG_DEFAULT_VERSION) == 0
						&& versionIndex == VER_NDX_INITIAL)) {
					return symbol;
				}

				// If not hidden, remember the version -- we'll return it, if
				// it is the only one.
				if ((versionID & VER_NDX_FLAG_HIDDEN) == 0) {
					versionedSymbolCount++;
					versionedSymbol = symbol;
				}
			}
		}
	}

	return versionedSymbolCount == 1 ? versionedSymbol : NULL;
}


status_t
find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo,
	void **_location)
{
	// get the symbol in the image
	Elf32_Sym* symbol = find_symbol(image, lookupInfo);
	if (symbol == NULL)
		return B_ENTRY_NOT_FOUND;

	void* location = (void*)(symbol->st_value + image->regions[0].delta);
	int32 symbolType = lookupInfo.type;
	patch_defined_symbol(image, lookupInfo.name, &location, &symbolType);

	if (_location != NULL)
		*_location = location;

	return B_OK;
}


status_t
find_symbol_breadth_first(image_t* image, const SymbolLookupInfo& lookupInfo,
	image_t** _foundInImage, void** _location)
{
	image_t* queue[count_loaded_images()];
	uint32 count = 0;
	uint32 index = 0;
	queue[count++] = image;
	image->flags |= RFLAG_VISITED;

	bool found = false;
	while (index < count) {
		// pop next image
		image = queue[index++];

		if (find_symbol(image, lookupInfo, _location) == B_OK) {
			found = true;
			break;
		}

		// push needed images
		for (uint32 i = 0; i < image->num_needed; i++) {
			image_t* needed = image->needed[i];
			if ((needed->flags & RFLAG_VISITED) == 0) {
				queue[count++] = needed;
				needed->flags |= RFLAG_VISITED;
			}
		}
	}

	// clear visited flags
	for (uint32 i = 0; i < count; i++)
		queue[i]->flags &= ~RFLAG_VISITED;

	return found ? B_OK : B_ENTRY_NOT_FOUND;
}


Elf32_Sym*
find_undefined_symbol_beos(image_t* rootImage, image_t* image,
	const SymbolLookupInfo& lookupInfo, image_t** foundInImage)
{
	// BeOS style symbol resolution: It is sufficient to check the direct
	// dependencies. The linker would have complained, if the symbol wasn't
	// there.
	for (uint32 i = 0; i < image->num_needed; i++) {
		if (image->needed[i]->dynamic_ptr) {
			Elf32_Sym *symbol = find_symbol(image->needed[i],
				lookupInfo);
			if (symbol) {
				*foundInImage = image->needed[i];
				return symbol;
			}
		}
	}

	return NULL;
}


Elf32_Sym*
find_undefined_symbol_global(image_t* rootImage, image_t* image,
	const SymbolLookupInfo& lookupInfo, image_t** foundInImage)
{
	// Global load order symbol resolution: All loaded images are searched for
	// the symbol in the order they have been loaded. We skip add-on images and
	// RTLD_LOCAL images though.
	image_t* otherImage = get_loaded_images().head;
	while (otherImage != NULL) {
		if (otherImage == rootImage
			|| (otherImage->type != B_ADD_ON_IMAGE
				&& (otherImage->flags
					& (RTLD_GLOBAL | RFLAG_USE_FOR_RESOLVING)) != 0)) {
			Elf32_Sym *symbol = find_symbol(otherImage, lookupInfo);
			if (symbol) {
				*foundInImage = otherImage;
				return symbol;
			}
		}
		otherImage = otherImage->next;
	}

	return NULL;
}


Elf32_Sym*
find_undefined_symbol_add_on(image_t* rootImage, image_t* image,
	const SymbolLookupInfo& lookupInfo, image_t** foundInImage)
{
// TODO: How do we want to implement this one? Using global scope resolution
// might be undesired as it is now, since libraries could refer to symbols in
// the add-on, which would result in add-on symbols implicitely becoming used
// outside of the add-on. So the options would be to use the global scope but
// skip the add-on, or to do breadth-first resolution in the add-on dependency
// scope, also skipping the add-on itself. BeOS style resolution is safe, too,
// but we miss out features like undefined symbols and preloading.
	return find_undefined_symbol_beos(rootImage, image, lookupInfo,
		foundInImage);
}


int
resolve_symbol(image_t* rootImage, image_t* image, struct Elf32_Sym* sym,
	addr_t* symAddress)
{
	switch (sym->st_shndx) {
		case SHN_UNDEF:
		{
			struct Elf32_Sym* sharedSym;
			image_t* sharedImage;
			const char* symName = SYMNAME(image, sym);

			// patch the symbol name
			if (image->abi < B_HAIKU_ABI_GCC_2_HAIKU) {
				// The image has been compiled with a BeOS compiler. This means
				// we'll have to redirect some functions for compatibility.
				symName = beos_compatibility_map_symbol(symName);
			}

			// get the version info
			const elf_version_info* versionInfo = NULL;
			if (image->symbol_versions != NULL) {
				uint32 index = sym - image->syms;
				uint32 versionIndex = VER_NDX(image->symbol_versions[index]);
				if (versionIndex >= VER_NDX_INITIAL)
					versionInfo = image->versions + versionIndex;
			}

			int32 type = B_SYMBOL_TYPE_ANY;
			if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
				type = B_SYMBOL_TYPE_TEXT;
			else if (ELF32_ST_TYPE(sym->st_info) == STT_OBJECT)
				type = B_SYMBOL_TYPE_DATA;

			// it's undefined, must be outside this image, try the other images
			sharedSym = rootImage->find_undefined_symbol(rootImage, image,
				SymbolLookupInfo(symName, type, versionInfo), &sharedImage);
			void* location = NULL;

			enum {
				ERROR_NO_SYMBOL,
				ERROR_WRONG_TYPE,
				ERROR_NOT_EXPORTED,
				ERROR_UNPATCHED
			};
			uint32 lookupError = ERROR_UNPATCHED;

			if (sharedSym == NULL) {
				// symbol not found at all
				lookupError = ERROR_NO_SYMBOL;
				sharedImage = NULL;
			} else if (ELF32_ST_TYPE(sym->st_info) != STT_NOTYPE
				&& ELF32_ST_TYPE(sym->st_info)
					!= ELF32_ST_TYPE(sharedSym->st_info)) {
				// symbol not of the requested type
				lookupError = ERROR_WRONG_TYPE;
				sharedImage = NULL;
			} else if (ELF32_ST_BIND(sharedSym->st_info) != STB_GLOBAL
				&& ELF32_ST_BIND(sharedSym->st_info) != STB_WEAK) {
				// symbol not exported
				lookupError = ERROR_NOT_EXPORTED;
				sharedImage = NULL;
			} else {
				// symbol is fine, get its location
				location = (void*)(sharedSym->st_value
					+ sharedImage->regions[0].delta);
			}

			patch_undefined_symbol(rootImage, image, symName, &sharedImage,
				&location, &type);

			if (location == NULL) {
				switch (lookupError) {
					case ERROR_NO_SYMBOL:
						FATAL("elf_resolve_symbol: could not resolve symbol "
							"'%s'\n", symName);
						break;
					case ERROR_WRONG_TYPE:
						FATAL("elf_resolve_symbol: found symbol '%s' in shared "
							"image but wrong type\n", symName);
						break;
					case ERROR_NOT_EXPORTED:
						FATAL("elf_resolve_symbol: found symbol '%s', but not "
							"exported\n", symName);
						break;
					case ERROR_UNPATCHED:
						FATAL("elf_resolve_symbol: found symbol '%s', but was "
							"hidden by symbol patchers\n", symName);
						break;
				}

				if (report_errors())
					gErrorMessage.AddString("missing symbol", symName);

				return B_MISSING_SYMBOL;
			}

			*symAddress = (addr_t)location;
			return B_OK;
		}

		case SHN_ABS:
			*symAddress = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;

		case SHN_COMMON:
			// ToDo: finish this
			FATAL("elf_resolve_symbol: COMMON symbol, finish me!\n");
			return B_ERROR; //ERR_NOT_IMPLEMENTED_YET;

		default:
			// standard symbol
			*symAddress = sym->st_value + image->regions[0].delta;
			return B_NO_ERROR;
	}
}
