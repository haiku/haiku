/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SymbolLookup.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <new>

#include <runtime_loader.h>
#include <syscalls.h>

#include "Image.h"


#undef TRACE
//#define TRACE_DEBUG_SYMBOL_LOOKUP
#ifdef TRACE_DEBUG_SYMBOL_LOOKUP
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


using namespace BPrivate::Debug;


// PrepareAddress
const void *
Area::PrepareAddress(const void *address)
{
	TRACE(("Area::PrepareAddress(%p): area: %" B_PRId32 "\n", address, fRemoteID));

	// clone the area, if not done already
	if (fLocalID < 0) {
		fLocalID = clone_area("cloned area", &fLocalAddress, B_ANY_ADDRESS,
			B_READ_AREA, fRemoteID);
		if (fLocalID < 0) {
			TRACE(("Area::PrepareAddress(): Failed to clone area %" B_PRId32
				": %s\n", fRemoteID, strerror(fLocalID)));
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
	// If the team is the kernel team, we don't try to clone the areas. Only
	// SymbolLookup's image file functionality will be available.
	if (fTeam == B_SYSTEM_TEAM)
		return B_OK;

	// get a list of the team's areas
	area_info areaInfo;
	ssize_t cookie = 0;
	status_t error;
	while ((error = get_next_area_info(fTeam, &cookie, &areaInfo)) == B_OK) {
		TRACE(("area %" B_PRId32 ": address: %p, size: %ld, name: %s\n",
			areaInfo.area, areaInfo.address, areaInfo.size, areaInfo.name));

		Area *area = new(std::nothrow) Area(areaInfo.area, areaInfo.address,
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
RemoteMemoryAccessor::PrepareAddress(const void *remoteAddress,
	int32 size) const
{
	TRACE(("RemoteMemoryAccessor::PrepareAddress(%p, %" B_PRId32 ")\n",
		remoteAddress, size));

	if (!remoteAddress) {
		TRACE(("RemoteMemoryAccessor::PrepareAddress(): Got null address!\n"));
		throw Exception(B_BAD_VALUE);
	}

	return _FindArea(remoteAddress, size).PrepareAddress(remoteAddress);
}


const void *
RemoteMemoryAccessor::PrepareAddressNoThrow(const void *remoteAddress,
	int32 size) const
{
	if (remoteAddress == NULL)
		return NULL;

	Area* area = _FindAreaNoThrow(remoteAddress, size);
	if (area == NULL)
		return NULL;

	return area->PrepareAddress(remoteAddress);
}


// AreaForLocalAddress
Area*
RemoteMemoryAccessor::AreaForLocalAddress(const void* address) const
{
	if (address == NULL)
		return NULL;

	for (AreaList::ConstIterator it = fAreas.GetIterator(); it.HasNext();) {
		Area* area = it.Next();
		if (area->ContainsLocalAddress(address))
			return area;
	}

	return NULL;
}


// _FindArea
Area &
RemoteMemoryAccessor::_FindArea(const void *address, int32 size) const
{
	TRACE(("RemoteMemoryAccessor::_FindArea(%p, %" B_PRId32 ")\n", address,
		size));

	for (AreaList::ConstIterator it = fAreas.GetIterator(); it.HasNext();) {
		Area *area = it.Next();
		if (area->ContainsAddress(address, size))
			return *area;
	}

	TRACE(("RemoteMemoryAccessor::_FindArea(): No area found for address %p\n",
		address));
	throw Exception(B_ENTRY_NOT_FOUND);
}


// _FindAreaNoThrow
Area*
RemoteMemoryAccessor::_FindAreaNoThrow(const void *address, int32 size) const
{
	for (AreaList::ConstIterator it = fAreas.GetIterator(); it.HasNext();) {
		Area *area = it.Next();
		if (area->ContainsAddress(address, size))
			return area;
	}

	return NULL;
}


// #pragma mark -


class SymbolLookup::LoadedImage : public Image {
public:
								LoadedImage(SymbolLookup* symbolLookup,
									const image_t* image, int32 symbolCount);
	virtual						~LoadedImage();

	virtual	const elf_sym*		LookupSymbol(addr_t address,
									addr_t* _baseAddress,
									const char** _symbolName,
									size_t *_symbolNameLen,
									bool *_exactMatch) const;
	virtual	status_t			NextSymbol(int32& iterator,
									const char** _symbolName,
									size_t* _symbolNameLen,
									addr_t* _symbolAddress, size_t* _symbolSize,
									int32* _symbolType) const;

private:
			SymbolLookup*			fSymbolLookup;
			const image_t*			fImage;
			int32					fSymbolCount;
			size_t					fTextDelta;
};


// #pragma mark -


// constructor
SymbolLookup::SymbolLookup(team_id team)
	:
	RemoteMemoryAccessor(team),
	fDebugArea(NULL),
	fImages()
{
}


// destructor
SymbolLookup::~SymbolLookup()
{
	while (Image* image = fImages.RemoveHead())
		delete image;
}


// Init
status_t
SymbolLookup::Init()
{
	TRACE(("SymbolLookup::Init()\n"));

	status_t error = RemoteMemoryAccessor::Init();
	if (error != B_OK)
		return error;

	if (fTeam != B_SYSTEM_TEAM) {
		TRACE(("SymbolLookup::Init(): searching debug area...\n"));

		// find the runtime loader debug area
		runtime_loader_debug_area *remoteDebugArea = NULL;
		ssize_t cookie = 0;
		area_info areaInfo;
		while (get_next_area_info(fTeam, &cookie, &areaInfo) == B_OK) {
			if (strcmp(areaInfo.name, RUNTIME_LOADER_DEBUG_AREA_NAME) == 0) {
				remoteDebugArea = (runtime_loader_debug_area*)areaInfo.address;
				break;
			}
		}

		if (remoteDebugArea) {
			TRACE(("SymbolLookup::Init(): found debug area, translating "
				"address...\n"));
		} else {
			TRACE(("SymbolLookup::Init(): Couldn't find debug area!\n"));
		}

		// translate the address
		try {
			if (remoteDebugArea != NULL) {
				fDebugArea = &Read(*remoteDebugArea);

				TRACE(("SymbolLookup::Init(): translated debug area is at: %p, "
					"loaded_images: %p\n", fDebugArea, fDebugArea->loaded_images));
			}
		} catch (Exception exception) {
			// we can live without the debug area
		}
	}

	// create a list of the team's images
	image_info imageInfo;
	int32 cookie = 0;
	while (get_next_image_info(fTeam, &cookie, &imageInfo) == B_OK) {
		Image* image;

		if (fTeam == B_SYSTEM_TEAM) {
			// kernel image
			KernelImage* kernelImage = new(std::nothrow) KernelImage;
			if (kernelImage == NULL)
				return B_NO_MEMORY;

			error = kernelImage->Init(imageInfo);
			image = kernelImage;
		} else {
			// userland image -- try to load an image file
			ImageFile* imageFile = new(std::nothrow) ImageFile;
			if (imageFile == NULL)
				return B_NO_MEMORY;

			error = imageFile->Init(imageInfo);
			image = imageFile;
		}

		if (error != B_OK) {
			// initialization error -- fall back to the loaded image
			delete image;

			const image_t* loadedImage = _FindLoadedImageByID(imageInfo.id);
			if (loadedImage == NULL)
				continue;

			image = new(std::nothrow) LoadedImage(this, loadedImage,
				Read(loadedImage->symhash[1]));
			if (image == NULL)
				return B_NO_MEMORY;
		}

		fImages.Add(image);
	}

	return B_OK;
}


// LookupSymbolAddress
status_t
SymbolLookup::LookupSymbolAddress(addr_t address, addr_t *_baseAddress,
	const char **_symbolName, size_t *_symbolNameLen, const char **_imageName,
	bool *_exactMatch) const
{
	TRACE(("SymbolLookup::LookupSymbolAddress(%p)\n", (void*)address));

	Image* image = _FindImageAtAddress(address);
	if (!image)
		return B_ENTRY_NOT_FOUND;

	if (_imageName != NULL)
		*_imageName = image->Name();

	const elf_sym* symbolFound = image->LookupSymbol(address, _baseAddress,
		_symbolName, _symbolNameLen, _exactMatch);

	TRACE(("SymbolLookup::LookupSymbolAddress(): done: symbol: %p, image name: "
		"%s, exact match: %d\n", symbolFound, image->name, exactMatch));

	if (symbolFound != NULL)
		return B_OK;

	// symbol not found -- return the image itself

	if (_baseAddress)
		*_baseAddress = image->TextAddress();

	if (_imageName)
		*_imageName = image->Name();

	if (_symbolName)
		*_symbolName = NULL;

	if (_exactMatch)
		*_exactMatch = false;

	if (_symbolNameLen != NULL)
		*_symbolNameLen = 0;

	return B_OK;
}


// InitSymbolIterator
status_t
SymbolLookup::InitSymbolIterator(image_id imageID,
	SymbolIterator& iterator) const
{
	TRACE(("SymbolLookup::InitSymbolIterator(): image ID: %" B_PRId32 "\n",
		imageID));

	// find the image
	iterator.image = _FindImageByID(imageID);

	// If that didn't work, find the loaded image.
	if (iterator.image == NULL) {
		TRACE(("SymbolLookup::InitSymbolIterator() done: image not "
			"found\n"));
		return B_ENTRY_NOT_FOUND;
	}

	iterator.currentIndex = -1;

	return B_OK;
}


// InitSymbolIterator
status_t
SymbolLookup::InitSymbolIteratorByAddress(addr_t address,
	SymbolIterator& iterator) const
{
	TRACE(("SymbolLookup::InitSymbolIteratorByAddress(): base address: %#lx\n",
		address));

	// find the image
	iterator.image = _FindImageAtAddress(address);
	if (iterator.image == NULL) {
		TRACE(("SymbolLookup::InitSymbolIteratorByAddress() done: image "
			"not found\n"));
		return B_ENTRY_NOT_FOUND;
	}

	iterator.currentIndex = -1;

	return B_OK;
}


// NextSymbol
status_t
SymbolLookup::NextSymbol(SymbolIterator& iterator, const char** _symbolName,
	size_t* _symbolNameLen, addr_t* _symbolAddress, size_t* _symbolSize,
	int32* _symbolType) const
{
	return iterator.image->NextSymbol(iterator.currentIndex, _symbolName,
		_symbolNameLen, _symbolAddress, _symbolSize, _symbolType);
}


// GetSymbol
status_t
SymbolLookup::GetSymbol(image_id imageID, const char* name, int32 symbolType,
	void** _symbolLocation, size_t* _symbolSize, int32* _symbolType) const
{
	Image* image = _FindImageByID(imageID);
	if (image == NULL)
		return B_ENTRY_NOT_FOUND;

	return image->GetSymbol(name, symbolType, _symbolLocation, _symbolSize,
		_symbolType);
}


// _FindLoadedImageAtAddress
const image_t *
SymbolLookup::_FindLoadedImageAtAddress(addr_t address) const
{
	TRACE(("SymbolLookup::_FindLoadedImageAtAddress(%p)\n", (void*)address));

	if (fDebugArea == NULL)
		return NULL;

	// iterate through the loaded images
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


// _FindLoadedImageByID
const image_t*
SymbolLookup::_FindLoadedImageByID(image_id id) const
{
	if (fDebugArea == NULL)
		return NULL;

	// iterate through the images
	for (const image_t *image = &Read(*Read(fDebugArea->loaded_images->head));
		 image;
		 image = &Read(*image->next)) {
		if (image->id == id)
			return image;
	}

	return NULL;
}


// _FindImageAtAddress
Image*
SymbolLookup::_FindImageAtAddress(addr_t address) const
{
	DoublyLinkedList<Image>::ConstIterator it = fImages.GetIterator();
	while (Image* image = it.Next()) {
		addr_t textAddress = image->TextAddress();
		if (address >= textAddress && address < textAddress + image->TextSize())
			return image;
	}

	return NULL;
}


// _FindImageByID
Image*
SymbolLookup::_FindImageByID(image_id id) const
{
	DoublyLinkedList<Image>::ConstIterator it = fImages.GetIterator();
	while (Image* image = it.Next()) {
		if (image->ID() == id)
			return image;
	}

	return NULL;
}


// _SymbolNameLen
size_t
SymbolLookup::_SymbolNameLen(const char* address) const
{
	Area* area = AreaForLocalAddress(address);
	if (area == NULL)
		return 0;

	return strnlen(address, (addr_t)area->LocalAddress() + area->Size()
		- (addr_t)address);
}


// #pragma mark - LoadedImage


SymbolLookup::LoadedImage::LoadedImage(SymbolLookup* symbolLookup,
	const image_t* image, int32 symbolCount)
	:
	fSymbolLookup(symbolLookup),
	fImage(image),
	fSymbolCount(symbolCount),
	fTextDelta(image->regions[0].delta)
{
	// init info
	fInfo.id = fImage->id;
	fInfo.type = fImage->type;
	fInfo.sequence = 0;
	fInfo.init_order = 0;
	fInfo.init_routine = (void (*)())fImage->init_routine;
	fInfo.term_routine = (void (*)())fImage->term_routine;
	fInfo.device = -1;
	fInfo.node = -1;
	strlcpy(fInfo.name, fImage->path, sizeof(fInfo.name));
	fInfo.text = (void*)fImage->regions[0].vmstart;
	fInfo.data = (void*)fImage->regions[1].vmstart;
	fInfo.text_size = fImage->regions[0].vmsize;
	fInfo.data_size = fImage->regions[1].vmsize;
}


SymbolLookup::LoadedImage::~LoadedImage()
{
}


const elf_sym*
SymbolLookup::LoadedImage::LookupSymbol(addr_t address, addr_t* _baseAddress,
	const char** _symbolName, size_t *_symbolNameLen, bool *_exactMatch) const
{
	TRACE(("LoadedImage::LookupSymbol(): found image: ID: %" B_PRId32 ", text: "
		"address: %p, size: %ld\n", fImage->id,
		(void*)fImage->regions[0].vmstart, fImage->regions[0].size));

	// search the image for the symbol
	const elf_sym *symbolFound = NULL;
	addr_t deltaFound = INT_MAX;
	bool exactMatch = false;
	const char *symbolName = NULL;

	int32 symbolCount = fSymbolLookup->Read(fImage->symhash[1]);
	const elf_region_t *textRegion = fImage->regions;				// local

	for (int32 i = 0; i < symbolCount; i++) {
		const elf_sym *symbol = &fSymbolLookup->Read(fImage->syms[i]);

		// The symbol table contains not only symbols referring to functions
		// and data symbols within the shared object, but also referenced
		// symbols of other shared objects, as well as section and file
		// references. We ignore everything but function and data symbols
		// that have an st_value != 0 (0 seems to be an indication for a
		// symbol defined elsewhere -- couldn't verify that in the specs
		// though).
		if ((symbol->Type() != STT_FUNC && symbol->Type() != STT_OBJECT)
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
			symbolName = (const char*)fSymbolLookup->PrepareAddressNoThrow(
				SYMNAME(fImage, symbol), 1);
			if (symbolName == NULL)
				continue;

			deltaFound = symbolDelta;
			symbolFound = symbol;

			if (symbolDelta >= 0 && symbolDelta < symbol->st_size) {
				// exact match
				exactMatch = true;
				break;
			}
		}
	}

	TRACE(("LoadedImage::LookupSymbol(): done: symbol: %p, image name: "
		"%s, exact match: %d\n", symbolFound, fImage->name, exactMatch));

	if (symbolFound != NULL) {
		if (_baseAddress)
			*_baseAddress = symbolFound->st_value + textRegion->delta;
		if (_symbolName)
			*_symbolName = symbolName;
		if (_exactMatch)
			*_exactMatch = exactMatch;
		if (_symbolNameLen != NULL)
			*_symbolNameLen = fSymbolLookup->_SymbolNameLen(symbolName);
	}

	return symbolFound;
}


status_t
SymbolLookup::LoadedImage::NextSymbol(int32& iterator, const char** _symbolName,
	size_t* _symbolNameLen, addr_t* _symbolAddress, size_t* _symbolSize,
	int32* _symbolType) const
{
	while (true) {
		if (++iterator >= fSymbolCount)
			return B_ENTRY_NOT_FOUND;

		const elf_sym* symbol
			= &fSymbolLookup->Read(fImage->syms[iterator]);
		if ((symbol->Type() != STT_FUNC && symbol->Type() != STT_OBJECT)
			|| symbol->st_value == 0) {
			continue;
		}

		*_symbolName = (const char*)fSymbolLookup->PrepareAddressNoThrow(
			SYMNAME(fImage, symbol), 1);
		*_symbolNameLen = fSymbolLookup->_SymbolNameLen(*_symbolName);
		*_symbolAddress = symbol->st_value + fTextDelta;
		*_symbolSize = symbol->st_size;
		*_symbolType = symbol->Type() == STT_FUNC ? B_SYMBOL_TYPE_TEXT
			: B_SYMBOL_TYPE_DATA;

		return B_OK;
	}
}
