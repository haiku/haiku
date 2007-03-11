/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "SymbolLookup.h"

#include <new>

#include <string.h>
#include <runtime_loader.h>


using std::nothrow;
using namespace BPrivate;

// PrepareAddress
const void *
Area::PrepareAddress(const void *address)
{
	TRACE(("Area::PrepareAddress(%p): area: %ld\n", address, fRemoteID));

	// clone the area, if not done already
	if (fLocalID < 0) {
		fLocalID = clone_area("cloned area", &fLocalAddress, B_ANY_ADDRESS,
			B_READ_AREA, fRemoteID);
		if (fLocalID < 0) {
			TRACE(("Area::PrepareAddress(): Failed to clone area %ld: %s\n",
				fRemoteID, strerror(fLocalID)));
			throw Exception(fLocalID);
		}
	}

	// translate the address
	const void *result = (const void*)((addr_t)address - (addr_t)fRemoteAddress
		+ (addr_t)fLocalAddress);

	TRACE(("Area::PrepareAddress(%p) done: %p\n", address, result));

	return result;
}


// #pragma mark -

// constructor
RemoteMemoryAccessor::RemoteMemoryAccessor(team_id team)
	: fTeam(team),
	  fAreas()
{
}

// destructor
RemoteMemoryAccessor::~RemoteMemoryAccessor()
{
	// delete the areas
	while (Area *area = fAreas.Head()) {
		fAreas.Remove(area);
		delete area;
	}
}

// Init
status_t
RemoteMemoryAccessor::Init()
{
	// get a list of the team's areas
	area_info areaInfo;
	int32 cookie = 0;
	status_t error;
	while ((error = get_next_area_info(fTeam, &cookie, &areaInfo)) == B_OK) {

		TRACE(("area %ld: address: %p, size: %ld, name: %s\n", areaInfo.area,
			areaInfo.address, areaInfo.size, areaInfo.name));

		Area *area = new(nothrow) Area(areaInfo.area, areaInfo.address,
			areaInfo.size);
		if (!area)
			return B_NO_MEMORY;

		fAreas.Add(area);
	}

	if (fAreas.IsEmpty())
		return error;

	return B_OK;
}

// PrepareAddress
const void *
RemoteMemoryAccessor::PrepareAddress(const void *remoteAddress, int32 size)
{
	TRACE(("RemoteMemoryAccessor::PrepareAddress(%p, %ld)\n", remoteAddress,
		size));

	if (!remoteAddress) {
		TRACE(("RemoteMemoryAccessor::PrepareAddress(): Got null address!\n"));
		throw Exception(B_BAD_VALUE);
	}

	return _FindArea(remoteAddress, size).PrepareAddress(remoteAddress);
}

// _FindArea
Area &
RemoteMemoryAccessor::_FindArea(const void *address, int32 size)
{
	TRACE(("RemoteMemoryAccessor::_FindArea(%p, %ld)\n", address, size));

	for (AreaList::Iterator it = fAreas.GetIterator(); it.HasNext();) {
		Area *area = it.Next();
		if (area->ContainsAddress(address, size))
			return *area;
	}

	TRACE(("RemoteMemoryAccessor::_FindArea(): No area found for address %p\n",
		address));
	throw Exception(B_ENTRY_NOT_FOUND);
}


// #pragma mark -

// constructor
SymbolLookup::SymbolLookup(team_id team)
	: RemoteMemoryAccessor(team),
	  fDebugArea(NULL)
{
}

// destructor
SymbolLookup::~SymbolLookup()
{
}

// Init
status_t
SymbolLookup::Init()
{
	TRACE(("SymbolLookup::Init()\n"));

	status_t error = RemoteMemoryAccessor::Init();
	if (error != B_OK)
		return error;

	TRACE(("SymbolLookup::Init(): searching debug area...\n"));

	// find the runtime loader debug area
	runtime_loader_debug_area *remoteDebugArea = NULL;
	int32 cookie = 0;
	area_info areaInfo;
	while (get_next_area_info(fTeam, &cookie, &areaInfo) == B_OK) {
		if (strcmp(areaInfo.name, RUNTIME_LOADER_DEBUG_AREA_NAME) == 0) {
			remoteDebugArea = (runtime_loader_debug_area*)areaInfo.address;
			break;
		}
	}

	if (!remoteDebugArea) {
		TRACE(("SymbolLookup::Init(): Couldn't find debug area!\n"));
		return B_ERROR;
	}

	TRACE(("SymbolLookup::Init(): found debug area, translating address...\n"));

	// translate the address
	try {
		fDebugArea = &Read(*remoteDebugArea);

		TRACE(("SymbolLookup::Init(): translated debug area is at: %p, "
			"loaded_images: %p\n", fDebugArea, fDebugArea->loaded_images));

	} catch (Exception exception) {
		return exception.Error();
	}

	return B_OK;
}

// LookupSymbolAddress
status_t
SymbolLookup::LookupSymbolAddress(addr_t address, addr_t *_baseAddress,
	const char **_symbolName, const char **_imageName, bool *_exactMatch)
{
	// Note, that this function doesn't find all symbols that we would like
	// to find. E.g. static functions do not appear in the symbol table
	// as function symbols, but as sections without name and size. The .symtab
	// section together with the .strtab section, which apparently differ from
	// the tables referred to by the .dynamic section, also contain proper names
	// and sizes for those symbols. Therefore, to get completely satisfying
	// results, we would need to read those tables from the shared object.

	TRACE(("SymbolLookup::LookupSymbolAddress(%p)\n", (void*)address));

	// get the image for the address
	const image_t *image = _FindImageAtAddress(address);
	if (!image)
		return B_ENTRY_NOT_FOUND;

	TRACE(("SymbolLookup::LookupSymbolAddress(): found image: ID: %ld, text: "
		"address: %p, size: %ld\n",
		image->id, (void*)image->regions[0].vmstart, image->regions[0].size));

	// search the image for the symbol
	const struct Elf32_Sym *symbolFound = NULL;
	addr_t deltaFound = INT_MAX;
	bool exactMatch = false;
	const char *symbolName = NULL;	// remote

	int32 hashTabSize = Read(image->symhash[0]);
	const uint32 *hashBuckets = image->symhash + 2;					// remote
	const uint32 *hashChains = image->symhash + 2 + hashTabSize;	// remote
	const elf_region_t *textRegion = image->regions;				// local

	for (int32 i = 0; i < hashTabSize; i++) {
		for (int32 j = Read(hashBuckets[i]);
			 j != STN_UNDEF;
			 j = Read(hashChains[j])) {

			const struct Elf32_Sym *symbol = &Read(image->syms[j]);

			// The symbol table contains not only symbols referring to functions
			// and data symbols within the shared object, but also referenced
			// symbols of other shared objects, as well as section and file
			// references. We ignore everything but function and data symbols
			// that have an st_value != 0 (0 seems to be an indication for a
			// symbol defined elsewhere -- couldn't verify that in the specs
			// though).
			if ((ELF32_ST_TYPE(symbol->st_info) != STT_FUNC
					&& ELF32_ST_TYPE(symbol->st_info) != STT_OBJECT)
				|| symbol->st_value == 0
				|| symbol->st_value + symbol->st_size + textRegion->delta
					> textRegion->vmstart + textRegion->size) {
				continue;
			}

			// skip symbols starting after the given address
			addr_t symbolAddress = symbol->st_value + textRegion->delta;

			if (symbolAddress > address)
				continue;
			addr_t symbolDelta = address - symbolAddress;

			if (!symbolFound || symbolDelta < deltaFound) {
				deltaFound = symbolDelta;
				symbolFound = symbol;
				symbolName = SYMNAME(image, symbol);

				if (symbolDelta >= 0 && symbolDelta < symbol->st_size) {
					// exact match
					exactMatch = true;
					break;
				}
			}
		}
	}

	TRACE(("SymbolLookup::LookupSymbolAddress(): done: symbol: %p, image name: "
		"%s, exact match: %d\n", symbolFound, image->name, exactMatch));

	if (_baseAddress) {
		if (symbolFound)
			*_baseAddress = symbolFound->st_value + textRegion->delta;
		else
			*_baseAddress = textRegion->vmstart;
	}

	if (_imageName)
		*_imageName = image->name;

	if (_symbolName)
		*_symbolName = symbolName;	// remote address

	if (_exactMatch)
		*_exactMatch = exactMatch;

	return B_OK;
}

// _FindImageAtAddress
const image_t *
SymbolLookup::_FindImageAtAddress(addr_t address)
{
	TRACE(("SymbolLookup::_FindImageAtAddress(%p)\n", (void*)address));

	// iterate through the images
	for (const image_t *image = &Read(*Read(fDebugArea->loaded_images->head));
		 image;
		 image = &Read(*image->next)) {
		if (image->regions[0].vmstart <= address
			&& address < image->regions[0].vmstart + image->regions[0].size) {
			return image;
		}
	}

	return NULL;
}

