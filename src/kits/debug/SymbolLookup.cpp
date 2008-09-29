/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SymbolLookup.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <new>

#include <runtime_loader.h>
#include <syscalls.h>


#undef TRACE
//#define TRACE_DEBUG_SYMBOL_LOOKUP
#ifdef TRACE_DEBUG_SYMBOL_LOOKUP
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


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
	// If the team is the kernel team, we don't try to clone the areas. Only
	// SymbolLookup's image file functionality will be available.
	if (fTeam == B_SYSTEM_TEAM)
		return B_OK;

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
RemoteMemoryAccessor::PrepareAddress(const void *remoteAddress,
	int32 size) const
{
	TRACE(("RemoteMemoryAccessor::PrepareAddress(%p, %ld)\n", remoteAddress,
		size));

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
	TRACE(("RemoteMemoryAccessor::_FindArea(%p, %ld)\n", address, size));

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


ImageFile::ImageFile(const image_info& info)
	:
	fInfo(info),
	fFD(-1),
	fFileSize(0),
	fMappedFile((uint8*)MAP_FAILED),
	fLoadDelta(0),
	fSymbolTable(NULL),
	fStringTable(NULL),
	fSymbolCount(0),
	fStringTableSize(0)
{
}


ImageFile::~ImageFile()
{
	if (fMappedFile != MAP_FAILED) {
		munmap(fMappedFile, fFileSize);
	} else {
		delete[] fSymbolTable;
		delete[] fStringTable;
	}

	if (fFD >= 0)
		close(fFD);
}


status_t
ImageFile::Load()
{
	// open and stat() the file
	fFD = open(fInfo.name, O_RDONLY);
	if (fFD < 0)
		return errno;

	struct stat st;
	if (fstat(fFD, &st) < 0)
		return errno;

	fFileSize = st.st_size;
	if (fFileSize < sizeof(Elf32_Ehdr))
		return B_NOT_AN_EXECUTABLE;

	// map it
	fMappedFile = (uint8*)mmap(NULL, fFileSize, PROT_READ, MAP_PRIVATE, fFD, 0);
	if (fMappedFile == MAP_FAILED)
		return errno;

	// examine the elf header
	Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)fMappedFile;
	if (memcmp(elfHeader->e_ident, ELF_MAGIC, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_ident[4] != ELFCLASS32)
		return B_NOT_AN_EXECUTABLE;

	// verify the location of the program headers
	int32 programHeaderCount = elfHeader->e_phnum;
	if (elfHeader->e_phoff < sizeof(Elf32_Ehdr)
		|| elfHeader->e_phentsize < sizeof(Elf32_Phdr)
		|| elfHeader->e_phoff + programHeaderCount * elfHeader->e_phentsize
			> fFileSize) {
		return B_NOT_AN_EXECUTABLE;
	}

	Elf32_Phdr* programHeaders
		= (Elf32_Phdr*)(fMappedFile + elfHeader->e_phoff);

	// verify the location of the section headers
	int32 sectionCount = elfHeader->e_shnum;
	if (elfHeader->e_shoff < sizeof(Elf32_Ehdr)
		|| elfHeader->e_shentsize < sizeof(Elf32_Shdr)
		|| elfHeader->e_shoff + sectionCount * elfHeader->e_shentsize
			> fFileSize) {
		return B_NOT_AN_EXECUTABLE;
	}

	Elf32_Shdr* sectionHeaders
		= (Elf32_Shdr*)(fMappedFile + elfHeader->e_shoff);

	// find the first segment -- we need its relative offset
	for (int32 i = 0; i < programHeaderCount; i++) {
		Elf32_Phdr* header = (Elf32_Phdr*)
			((uint8*)programHeaders + i * elfHeader->e_phentsize);
		if (header->p_type == PT_LOAD) {
			fLoadDelta = (addr_t)fInfo.text - header->p_vaddr;
			break;
		}
	}

	// find the symbol table
	for (int32 i = 0; i < elfHeader->e_shnum; i++) {
		Elf32_Shdr* sectionHeader = (Elf32_Shdr*)
			((uint8*)sectionHeaders + i * elfHeader->e_shentsize);

		if (sectionHeader->sh_type == SHT_SYMTAB) {
			Elf32_Shdr& stringHeader = *(Elf32_Shdr*)
				((uint8*)sectionHeaders
					+ sectionHeader->sh_link * elfHeader->e_shentsize);

			if (stringHeader.sh_type != SHT_STRTAB)
				return B_BAD_DATA;

			if (sectionHeader->sh_offset + sectionHeader->sh_size > fFileSize
				|| stringHeader.sh_offset + stringHeader.sh_size > fFileSize) {
				return B_BAD_DATA;
			}

			fSymbolTable = (Elf32_Sym*)(fMappedFile + sectionHeader->sh_offset);
			fStringTable = (char*)(fMappedFile + stringHeader.sh_offset);
			fSymbolCount = sectionHeader->sh_size / sizeof(Elf32_Sym);
			fStringTableSize = stringHeader.sh_size;

			return B_OK;
		}
	}

	return B_BAD_DATA;
}


status_t
ImageFile::LoadKernel()
{
	// get the table sizes
	fSymbolCount = 0;
	fStringTableSize = 0;
	status_t error = _kern_read_kernel_image_symbols(fInfo.id,
		NULL, &fSymbolCount, NULL, &fStringTableSize, NULL);
	if (error != B_OK)
		return error;

	// allocate the tables
	fSymbolTable = new(std::nothrow) Elf32_Sym[fSymbolCount];
	fStringTable = new(std::nothrow) char[fStringTableSize];
	if (fSymbolTable == NULL || fStringTable == NULL)
		return B_NO_MEMORY;

	// get the info
	return _kern_read_kernel_image_symbols(fInfo.id,
		fSymbolTable, &fSymbolCount, fStringTable, &fStringTableSize,
		&fLoadDelta);
}


const Elf32_Sym*
ImageFile::LookupSymbol(addr_t address, addr_t* _baseAddress,
	const char** _symbolName, size_t *_symbolNameLen, bool *_exactMatch) const
{
	const Elf32_Sym* symbolFound = NULL;
	const char* symbolName = NULL;
	bool exactMatch = false;
	addr_t deltaFound = ~(addr_t)0;

	for (int32 i = 0; i < fSymbolCount; i++) {
		const Elf32_Sym* symbol = &fSymbolTable[i];

		if (symbol->st_value == 0
			|| symbol->st_size >= (size_t)fInfo.text_size + fInfo.data_size) {
			continue;
		}

		addr_t symbolAddress = symbol->st_value + fLoadDelta;
		if (symbolAddress > address)
			continue;

		addr_t symbolDelta = address - symbolAddress;
		if (symbolDelta >= 0 && symbolDelta < symbol->st_size)
			exactMatch = true;

		if (exactMatch || symbolDelta < deltaFound) {
			deltaFound = symbolDelta;
			symbolFound = symbol;
			symbolName = fStringTable + symbol->st_name;

			if (exactMatch)
				break;
		}
	}

	if (symbolFound != NULL) {
		if (_baseAddress != NULL)
			*_baseAddress = symbolFound->st_value + fLoadDelta;
		if (_symbolName != NULL)
			*_symbolName = symbolName;
		if (_exactMatch != NULL)
			*_exactMatch = exactMatch;
		if (_symbolNameLen != NULL)
			*_symbolNameLen = _SymbolNameLen(symbolName);
	}

	return symbolFound;
}


status_t
ImageFile::NextSymbol(int32& iterator, const char** _symbolName,
	size_t* _symbolNameLen, addr_t* _symbolAddress, size_t* _symbolSize,
	int32* _symbolType) const
{
	while (true) {
		if (++iterator >= fSymbolCount)
			return B_ENTRY_NOT_FOUND;

		const Elf32_Sym* symbol = &fSymbolTable[iterator];

		if ((ELF32_ST_TYPE(symbol->st_info) != STT_FUNC
				&& ELF32_ST_TYPE(symbol->st_info) != STT_OBJECT)
			|| symbol->st_value == 0) {
			continue;
		}

		*_symbolName = fStringTable + symbol->st_name;
		*_symbolNameLen = _SymbolNameLen(*_symbolName);
		*_symbolAddress = symbol->st_value + fLoadDelta;
		*_symbolSize = symbol->st_size;
		*_symbolType = ELF32_ST_TYPE(symbol->st_info) == STT_FUNC
			? B_SYMBOL_TYPE_TEXT : B_SYMBOL_TYPE_DATA;

		return B_OK;
	}
}


size_t
ImageFile::_SymbolNameLen(const char* symbolName) const
{
	if (symbolName == NULL || (addr_t)symbolName < (addr_t)fStringTable
		|| (addr_t)symbolName >= (addr_t)fStringTable + fStringTableSize) {
		return 0;
	}

	return strnlen(symbolName,
		(addr_t)fStringTable + fStringTableSize - (addr_t)symbolName);
}


// #pragma mark -


// constructor
SymbolLookup::SymbolLookup(team_id team)
	:
	RemoteMemoryAccessor(team),
	fDebugArea(NULL),
	fImageFiles()
{
}


// destructor
SymbolLookup::~SymbolLookup()
{
	while (ImageFile* imageFile = fImageFiles.RemoveHead())
		delete imageFile;
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
		int32 cookie = 0;
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
		ImageFile* imageFile = new(std::nothrow) ImageFile(imageInfo);
		if (imageFile == NULL)
			break;

		error = fTeam == B_SYSTEM_TEAM
			? imageFile->LoadKernel() : imageFile->Load();
		if (error != B_OK) {
			delete imageFile;
			continue;
		}

		fImageFiles.Add(imageFile);
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

	// Try the loaded image file first -- it also contains static symbols.
	ImageFile* imageFile = _FindImageFileAtAddress(address);
	if (imageFile != NULL) {
		if (_imageName != NULL)
			*_imageName = imageFile->Info().name;
		const Elf32_Sym* symbol = imageFile->LookupSymbol(address, _baseAddress,
			_symbolName, _symbolNameLen, _exactMatch);
		if (symbol != NULL)
			return B_OK;
	}

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
	const char *symbolName = NULL;

	int32 symbolCount = Read(image->symhash[1]);
	const elf_region_t *textRegion = image->regions;				// local

	for (int32 i = 0; i < symbolCount; i++) {
		const struct Elf32_Sym *symbol = &Read(image->syms[i]);

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
			symbolName = (const char*)PrepareAddressNoThrow(SYMNAME(image,
				symbol), 1);
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

	if (_symbolNameLen != NULL)
		*_symbolNameLen = _SymbolNameLen(symbolName);

	return B_OK;
}


// InitSymbolIterator
status_t
SymbolLookup::InitSymbolIterator(image_id imageID,
	SymbolIterator& iterator) const
{
	TRACE(("SymbolLookup::InitSymbolIterator(): image ID: %ld\n", imageID));

	// find the image file
	iterator.imageFile = _FindImageFileByID(imageID);

	// If that didn't work, find the image.
	if (iterator.imageFile == NULL) {
		const image_t* image = _FindImageByID(imageID);
		if (image == NULL) {
			TRACE(("SymbolLookup::InitSymbolIterator() done: image not "
				"found\n"));
			return B_ENTRY_NOT_FOUND;
		}

		iterator.image = image;
		iterator.symbolCount = Read(image->symhash[1]);
		iterator.textDelta = image->regions->delta;
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

	// find the image file
	iterator.imageFile = _FindImageFileAtAddress(address);

	// If that didn't work, find the image.
	if (iterator.imageFile == NULL) {
		const image_t *image = _FindImageAtAddress(address);
		if (image == NULL) {
			TRACE(("SymbolLookup::InitSymbolIteratorByAddress() done: image "
				"not found\n"));
			return B_ENTRY_NOT_FOUND;
		}

		iterator.image = image;
		iterator.symbolCount = Read(image->symhash[1]);
		iterator.textDelta = image->regions->delta;
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
	// If we have an image file, get the next symbol from it.
	const ImageFile* imageFile = iterator.imageFile;
	if (imageFile != NULL) {
		return imageFile->NextSymbol(iterator.currentIndex, _symbolName,
			_symbolNameLen, _symbolAddress, _symbolSize, _symbolType);
	}

	// Otherwise we've to fall be to iterating through the image.
	const image_t* image = iterator.image;

	while (true) {
		if (++iterator.currentIndex >= iterator.symbolCount)
			return B_ENTRY_NOT_FOUND;

		const struct Elf32_Sym* symbol = &Read(image->syms[
			iterator.currentIndex]);
		if ((ELF32_ST_TYPE(symbol->st_info) != STT_FUNC
				&& ELF32_ST_TYPE(symbol->st_info) != STT_OBJECT)
			|| symbol->st_value == 0) {
			continue;
		}

		*_symbolName = (const char*)PrepareAddressNoThrow(SYMNAME(image,
			symbol), 1);
		*_symbolNameLen = _SymbolNameLen(*_symbolName);
		*_symbolAddress = symbol->st_value + iterator.textDelta;
		*_symbolSize = symbol->st_size;
		*_symbolType = ELF32_ST_TYPE(symbol->st_info) == STT_FUNC
			? B_SYMBOL_TYPE_TEXT : B_SYMBOL_TYPE_DATA;

		return B_OK;
	}
}


// _FindImageAtAddress
const image_t *
SymbolLookup::_FindImageAtAddress(addr_t address) const
{
	TRACE(("SymbolLookup::_FindImageAtAddress(%p)\n", (void*)address));

	if (fDebugArea == NULL)
		return NULL;

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


// _FindImageByID
const image_t*
SymbolLookup::_FindImageByID(image_id id) const
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


// _FindImageFileAtAddress
ImageFile*
SymbolLookup::_FindImageFileAtAddress(addr_t address) const
{
	DoublyLinkedList<ImageFile>::ConstIterator it = fImageFiles.GetIterator();
	while (ImageFile* imageFile = it.Next()) {
		const image_info& info = imageFile->Info();
		if (address >= (addr_t)info.text
			&& address < (addr_t)info.text + info.text_size) {
			return imageFile;
		}
	}

	return NULL;
}


// _FindImageFileByID
ImageFile*
SymbolLookup::_FindImageFileByID(image_id id) const
{
	DoublyLinkedList<ImageFile>::ConstIterator it = fImageFiles.GetIterator();
	while (ImageFile* imageFile = it.Next()) {
		if (imageFile->Info().id == id)
			return imageFile;
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
