/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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


/*!	Checks whether \a name matches the name of \a image.

	It is expected that \a name does not contain directory components. It is
	compared with the base name of \a image's name.

	\param image The image.
	\param name The name to check against. Can be NULL, in which case \c false
		is returned.
	\return \c true, iff \a name is non-NULL and matches the name of \a image.
*/
static bool
equals_image_name(const image_t* image, const char* name)
{
	if (name == NULL)
		return false;

	const char* lastSlash = strrchr(name, '/');
	return strcmp(image->name, lastSlash != NULL ? lastSlash + 1 : name) == 0;
}


// #pragma mark -


uint32
elf_hash(const char* _name)
{
	const uint8* name = (const uint8*)_name;

	uint32 h = 0;
	while (*name != '\0') {
		h = (h << 4) + *name++;
		h ^= (h >> 24) & 0xf0;
	}
	return (h & 0x0fffffff);
}


uint32
elf_gnuhash(const char* _name)
{
	const uint8* name = (const uint8*)_name;

	uint32 h = 5381;
	for (uint8 c = *name; c != '\0'; c = *++name)
		h = (h * 33) + c;

	return h;
}


struct match_result {
	elf_sym* symbol;
	elf_sym* versioned_symbol;
	uint32 versioned_symbol_count;

	match_result() : symbol(NULL), versioned_symbol(NULL), versioned_symbol_count(0) {}
};


static bool
match_symbol(const image_t* image, const SymbolLookupInfo& lookupInfo, uint32 symIdx,
	match_result& result)
{
	elf_sym* symbol = &image->syms[symIdx];
	if (symbol->st_shndx == SHN_UNDEF)
		return false;
	if (symbol->Bind() != STB_GLOBAL && symbol->Bind() != STB_WEAK)
		return false;

	// check if the type matches
	uint32 type = symbol->Type();
	if ((lookupInfo.type == B_SYMBOL_TYPE_TEXT && type != STT_FUNC)
		|| (lookupInfo.type == B_SYMBOL_TYPE_DATA
			&& type != STT_OBJECT && type != STT_FUNC)) {
		return false;
	}

	// check the symbol name
	if (strcmp(SYMNAME(image, symbol), lookupInfo.name) != 0)
		return false;

	// check the version

	// Handle the simple cases -- the image doesn't have version
	// information -- first.
	if (image->symbol_versions == NULL) {
		if (lookupInfo.version == NULL) {
			// No specific symbol version was requested either, so the
			// symbol is just fine.
			result.symbol = symbol;
			return true;
		}

		// A specific version is requested. If it's the dependency
		// referred to by the requested version, it's apparently an
		// older version of the dependency and we're not happy.
		if (equals_image_name(image, lookupInfo.version->file_name)) {
			// TODO: That should actually be kind of fatal!
			return false;
		}

		// This is some other image. We accept the symbol.
		result.symbol = symbol;
		return true;
	}

	// The image has version information. Let's see what we've got.
	uint32 versionID = image->symbol_versions[symIdx];
	uint32 versionIndex = VER_NDX(versionID);
	elf_version_info& version = image->versions[versionIndex];

	// skip local versions
	if (versionIndex == VER_NDX_LOCAL)
		return false;

	if (lookupInfo.version != NULL) {
		// a specific version is requested

		// compare the versions
		if (version.hash == lookupInfo.version->hash
			&& strcmp(version.name, lookupInfo.version->name) == 0) {
			// versions match
			result.symbol = symbol;
			return true;
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
			result.symbol = symbol;
			return true;
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
			result.symbol = symbol;
			return true;
		}

		// If not hidden, remember the version -- we'll return it, if
		// it is the only one.
		if ((versionID & VER_NDX_FLAG_HIDDEN) == 0) {
			result.versioned_symbol_count++;
			result.versioned_symbol = symbol;
		}
	}
	return false;
}


static elf_sym*
find_symbol_gnuhash(const image_t* image, const SymbolLookupInfo& lookupInfo)
{
	// Test against the Bloom filter.
	const uint32 wordSize = sizeof(elf_addr) * 8;
	const uint32 firstHash = lookupInfo.gnuhash & (wordSize - 1);
	const uint32 secondHash = lookupInfo.gnuhash >> image->gnuhash.shift2;
	const uint32 index = (lookupInfo.gnuhash / wordSize) & image->gnuhash.mask_words_count_mask;
	const elf_addr bloomWord = image->gnuhash.bloom[index];
	if (((bloomWord >> firstHash) & (bloomWord >> secondHash) & 1) == 0)
		return NULL;

	// Locate hash chain and corresponding value element.
	const uint32 bucket = image->gnuhash.buckets[lookupInfo.gnuhash % image->gnuhash.bucket_count];
	if (bucket == 0)
		return NULL;

	match_result result;
	const uint32* chain0 = image->gnuhash.chain0;
	const uint32* hashValue = &chain0[bucket];
	do {
		if (((*hashValue ^ lookupInfo.gnuhash) >> 1) != 0)
			continue;

		uint32 symIndex = hashValue - chain0;
		if (match_symbol(image, lookupInfo, symIndex, result))
			return result.symbol;
	} while ((*hashValue++ & 1) == 0);

	if (result.versioned_symbol_count == 1)
		return result.versioned_symbol;
	return NULL;
}


static elf_sym*
find_symbol_sysv(const image_t* image, const SymbolLookupInfo& lookupInfo)
{
	if (image->dynamic_ptr == 0)
		return NULL;

	match_result result;

	uint32 bucket = lookupInfo.hash % HASHTABSIZE(image);
	for (uint32 symIndex = HASHBUCKETS(image)[bucket]; symIndex != STN_UNDEF;
			symIndex = HASHCHAINS(image)[symIndex]) {
		if (match_symbol(image, lookupInfo, symIndex, result))
			return result.symbol;
	}

	if (result.versioned_symbol_count == 1)
		return result.versioned_symbol;
	return NULL;
}


elf_sym*
find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo)
{
	if (image->gnuhash.buckets != NULL) {
		if (lookupInfo.gnuhash == 0)
			const_cast<uint32&>(lookupInfo.gnuhash) = elf_gnuhash(lookupInfo.name);
		return find_symbol_gnuhash(image, lookupInfo);
	}

	if (lookupInfo.hash == 0)
		const_cast<uint32&>(lookupInfo.hash) = elf_hash(lookupInfo.name);
	return find_symbol_sysv(image, lookupInfo);
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


status_t
find_symbol(image_t* image, const SymbolLookupInfo& lookupInfo,
	void **_location)
{
	// get the symbol in the image
	elf_sym* symbol = find_symbol(image, lookupInfo);
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

	elf_sym* candidateSymbol = NULL;
	image_t* candidateImage = NULL;

	while (index < count) {
		// pop next image
		image = queue[index++];

		elf_sym* symbol = find_symbol(image, lookupInfo);
		if (symbol != NULL) {
			bool isWeak = symbol->Bind() == STB_WEAK;
			if (candidateImage == NULL || !isWeak) {
				candidateSymbol = symbol;
				candidateImage = image;

				if (!isWeak)
					break;
			}
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

	if (candidateSymbol == NULL)
		return B_ENTRY_NOT_FOUND;

	// compute the symbol location
	*_location = (void*)(candidateSymbol->st_value
		+ candidateImage->regions[0].delta);
	int32 symbolType = lookupInfo.type;
	patch_defined_symbol(candidateImage, lookupInfo.name, _location,
		&symbolType);

	if (_foundInImage != NULL)
		*_foundInImage = candidateImage;

	return B_OK;
}


elf_sym*
find_undefined_symbol_beos(image_t* rootImage, image_t* image,
	const SymbolLookupInfo& lookupInfo, image_t** foundInImage)
{
	// BeOS style symbol resolution: It is sufficient to check the image itself
	// and its direct dependencies. The linker would have complained, if the
	// symbol wasn't there. First we check whether the requesting symbol is
	// defined already -- then we can simply return it, since, due to symbolic
	// linking, that's the one we'd find anyway.
	if (elf_sym* symbol = lookupInfo.requestingSymbol) {
		if (symbol->st_shndx != SHN_UNDEF
			&& ((symbol->Bind() == STB_GLOBAL)
				|| (symbol->Bind() == STB_WEAK))) {
			*foundInImage = image;
			return symbol;
		}
	}

	// lookup in image
	elf_sym* symbol = find_symbol(image, lookupInfo);
	if (symbol != NULL) {
		*foundInImage = image;
		return symbol;
	}

	// lookup in dependencies
	for (uint32 i = 0; i < image->num_needed; i++) {
		if (image->needed[i]->dynamic_ptr) {
			symbol = find_symbol(image->needed[i], lookupInfo);
			if (symbol != NULL) {
				*foundInImage = image->needed[i];
				return symbol;
			}
		}
	}

	return NULL;
}


elf_sym*
find_undefined_symbol_global(image_t* rootImage, image_t* image,
	const SymbolLookupInfo& lookupInfo, image_t** _foundInImage)
{
	// Global load order symbol resolution: All loaded images are searched for
	// the symbol in the order they have been loaded. We skip add-on images and
	// RTLD_LOCAL images though.
	image_t* candidateImage = NULL;
	elf_sym* candidateSymbol = NULL;

	// If the requesting image is linked symbolically, look up the symbol there
	// first.
	bool symbolic = (image->flags & RFLAG_SYMBOLIC) != 0;
	if (symbolic) {
		candidateSymbol = find_symbol(image, lookupInfo);
		if (candidateSymbol != NULL) {
			if (candidateSymbol->Bind() != STB_WEAK) {
				*_foundInImage = image;
				return candidateSymbol;
			}

			candidateImage = image;
		}
	}

	image_t* otherImage = get_loaded_images().head;
	while (otherImage != NULL) {
		if (otherImage == rootImage
				? !symbolic
				: (otherImage->type != B_ADD_ON_IMAGE
					&& (otherImage->flags
						& (RTLD_GLOBAL | RFLAG_USE_FOR_RESOLVING)) != 0)) {
			if (elf_sym* symbol = find_symbol(otherImage, lookupInfo)) {
				*_foundInImage = otherImage;
				return symbol;
			}
		}
		otherImage = otherImage->next;
	}

	if (candidateSymbol != NULL)
		*_foundInImage = candidateImage;

	return candidateSymbol;
}


elf_sym*
find_undefined_symbol_add_on(image_t* rootImage, image_t* image,
	const SymbolLookupInfo& lookupInfo, image_t** _foundInImage)
{
	// Similar to global load order symbol resolution: All loaded images are
	// searched for the symbol in the order they have been loaded. We skip
	// add-on images and RTLD_LOCAL images though. The root image (i.e. the
	// add-on image) is skipped, too, but for the add-on itself we look up
	// a symbol that hasn't been found anywhere else in the add-on image.
	// The reason for skipping the add-on image is that we must not resolve
	// library symbol references to symbol definitions in the add-on, as
	// libraries can be shared between different add-ons and we must not
	// introduce connections between add-ons.

	// For the add-on image itself resolve non-weak symbols defined in the
	// add-on to themselves. This makes the symbol resolution order inconsistent
	// for those symbols, but avoids clashes of global symbols defined in the
	// add-on with symbols defined e.g. in the application. There's really the
	// same problem for weak symbols, but we don't have any way to discriminate
	// weak symbols that must be resolved globally from those that should be
	// resolved within the add-on.
	if (rootImage == image) {
		if (elf_sym* symbol = lookupInfo.requestingSymbol) {
			if (symbol->st_shndx != SHN_UNDEF
				&& (symbol->Bind() == STB_GLOBAL)) {
				*_foundInImage = image;
				return symbol;
			}
		}
	}

	image_t* candidateImage = NULL;
	elf_sym* candidateSymbol = NULL;

	// If the requesting image is linked symbolically, look up the symbol there
	// first.
	bool symbolic = (image->flags & RFLAG_SYMBOLIC) != 0;
	if (symbolic) {
		candidateSymbol = find_symbol(image, lookupInfo);
		if (candidateSymbol != NULL) {
			if (candidateSymbol->Bind() != STB_WEAK) {
				*_foundInImage = image;
				return candidateSymbol;
			}

			candidateImage = image;
		}
	}

	image_t* otherImage = get_loaded_images().head;
	while (otherImage != NULL) {
		if (otherImage != rootImage
			&& otherImage->type != B_ADD_ON_IMAGE
			&& (otherImage->flags
				& (RTLD_GLOBAL | RFLAG_USE_FOR_RESOLVING)) != 0) {
			if (elf_sym* symbol = find_symbol(otherImage, lookupInfo)) {
				if (symbol->Bind() != STB_WEAK) {
					*_foundInImage = otherImage;
					return symbol;
				}

				if (candidateSymbol == NULL) {
					candidateSymbol = symbol;
					candidateImage = otherImage;
				}
			}
		}
		otherImage = otherImage->next;
	}

	// If the symbol has not been found and we're trying to resolve a reference
	// in the add-on image, we also try to look it up there.
	if (!symbolic && candidateSymbol == NULL && image == rootImage) {
		candidateSymbol = find_symbol(image, lookupInfo);
		candidateImage = image;
	}

	if (candidateSymbol != NULL)
		*_foundInImage = candidateImage;

	return candidateSymbol;
}


int
resolve_symbol(image_t* rootImage, image_t* image, elf_sym* sym,
	SymbolLookupCache* cache, addr_t* symAddress, image_t** symbolImage)
{
	uint32 index = sym - image->syms;

	// check the cache first
	if (cache->IsSymbolValueCached(index)) {
		*symAddress = cache->SymbolValueAt(index, symbolImage);
		return B_OK;
	}

	elf_sym* sharedSym;
	image_t* sharedImage;
	const char* symName = SYMNAME(image, sym);

	// get the symbol type
	int32 type = B_SYMBOL_TYPE_ANY;
	if (sym->Type() == STT_FUNC)
		type = B_SYMBOL_TYPE_TEXT;

	if (sym->Bind() == STB_LOCAL) {
		// Local symbols references are always resolved to the given symbol.
		sharedImage = image;
		sharedSym = sym;
	} else {
		// get the version info
		const elf_version_info* versionInfo = NULL;
		if (image->symbol_versions != NULL) {
			uint32 versionIndex = VER_NDX(image->symbol_versions[index]);
			if (versionIndex >= VER_NDX_INITIAL)
				versionInfo = image->versions + versionIndex;
		}

		// search the symbol
		sharedSym = rootImage->find_undefined_symbol(rootImage, image,
			SymbolLookupInfo(symName, type, versionInfo, 0, sym), &sharedImage);
	}

	enum {
		SUCCESS,
		ERROR_NO_SYMBOL,
		ERROR_WRONG_TYPE,
		ERROR_NOT_EXPORTED,
		ERROR_UNPATCHED
	};
	uint32 lookupError = ERROR_UNPATCHED;

	bool tlsSymbol = sym->Type() == STT_TLS;
	void* location = NULL;
	if (sharedSym == NULL) {
		// symbol not found at all
		if (sym->Bind() == STB_WEAK) {
			// weak symbol: treat as NULL
			location = sharedImage = NULL;
		} else {
			lookupError = ERROR_NO_SYMBOL;
			sharedImage = NULL;
		}
	} else if (sym->Type() != STT_NOTYPE
		&& sym->Type() != sharedSym->Type()
		&& (sym->Type() != STT_OBJECT || sharedSym->Type() != STT_FUNC)) {
		// symbol not of the requested type, except object which can match function
		lookupError = ERROR_WRONG_TYPE;
		sharedImage = NULL;
	} else if (sharedSym->Bind() != STB_GLOBAL
		&& sharedSym->Bind() != STB_WEAK) {
		// symbol not exported
		lookupError = ERROR_NOT_EXPORTED;
		sharedImage = NULL;
	} else {
		// symbol is fine, get its location
		location = (void*)sharedSym->st_value;
		if (!tlsSymbol) {
			location
				= (void*)((addr_t)location + sharedImage->regions[0].delta);
		} else
			lookupError = SUCCESS;
	}

	if (!tlsSymbol) {
		patch_undefined_symbol(rootImage, image, symName, &sharedImage,
			&location, &type);
	}

	if (type == 0 || (location == NULL && sym->Bind() != STB_WEAK && lookupError != SUCCESS)) {
		switch (lookupError) {
			case ERROR_NO_SYMBOL:
				FATAL("%s: Could not resolve symbol '%s'\n",
					image->path, symName);
				break;
			case ERROR_WRONG_TYPE:
				FATAL("%s: Found symbol '%s' in shared image but wrong "
					"type\n", image->path, symName);
				break;
			case ERROR_NOT_EXPORTED:
				FATAL("%s: Found symbol '%s', but not exported\n",
					image->path, symName);
				break;
			case ERROR_UNPATCHED:
				FATAL("%s: Found symbol '%s', but was hidden by symbol "
					"patchers\n", image->path, symName);
				break;
		}

		if (report_errors())
			gErrorMessage.AddString("missing symbol", symName);

		return B_MISSING_SYMBOL;
	}

	cache->SetSymbolValueAt(index, (addr_t)location, sharedImage);

	if (symbolImage)
		*symbolImage = sharedImage;
	*symAddress = (addr_t)location;
	return B_OK;
}
