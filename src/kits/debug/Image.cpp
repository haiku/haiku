/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Image.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <new>

#include <runtime_loader.h>
#include <syscalls.h>


using namespace BPrivate::Debug;


// #pragma mark - Image


Image::Image()
{
}


Image::~Image()
{
}


status_t
Image::GetSymbol(const char* name, int32 symbolType, void** _symbolLocation,
	size_t* _symbolSize, int32* _symbolType) const
{
	// TODO: At least for ImageFile we could do hash lookups!
	int32 iterator = 0;
	const char* foundName;
	size_t foundNameLen;
	addr_t foundAddress;
	size_t foundSize;
	int32 foundType;
	while (NextSymbol(iterator, &foundName, &foundNameLen, &foundAddress,
			&foundSize, &foundType) == B_OK) {
		if ((symbolType == B_SYMBOL_TYPE_ANY || symbolType == foundType)
			&& strcmp(name, foundName) == 0) {
			if (_symbolLocation != NULL)
				*_symbolLocation = (void*)foundAddress;
			if (_symbolSize != NULL)
				*_symbolSize = foundSize;
			if (_symbolType != NULL)
				*_symbolType = foundType;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


// #pragma mark - SymbolTableBasedImage


SymbolTableBasedImage::SymbolTableBasedImage()
	:
	fLoadDelta(0),
	fSymbolTable(NULL),
	fStringTable(NULL),
	fSymbolCount(0),
	fStringTableSize(0)
{
}


SymbolTableBasedImage::~SymbolTableBasedImage()
{
}


const elf_sym*
SymbolTableBasedImage::LookupSymbol(addr_t address, addr_t* _baseAddress,
	const char** _symbolName, size_t *_symbolNameLen, bool *_exactMatch) const
{
	const elf_sym* symbolFound = NULL;
	const char* symbolName = NULL;
	bool exactMatch = false;
	addr_t deltaFound = ~(addr_t)0;

	for (int32 i = 0; i < fSymbolCount; i++) {
		const elf_sym* symbol = &fSymbolTable[i];

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
SymbolTableBasedImage::NextSymbol(int32& iterator, const char** _symbolName,
	size_t* _symbolNameLen, addr_t* _symbolAddress, size_t* _symbolSize,
	int32* _symbolType) const
{
	while (true) {
		if (++iterator >= fSymbolCount)
			return B_ENTRY_NOT_FOUND;

		const elf_sym* symbol = &fSymbolTable[iterator];

		if ((symbol->Type() != STT_FUNC && symbol->Type() != STT_OBJECT)
			|| symbol->st_value == 0) {
			continue;
		}

		*_symbolName = fStringTable + symbol->st_name;
		*_symbolNameLen = _SymbolNameLen(*_symbolName);
		*_symbolAddress = symbol->st_value + fLoadDelta;
		*_symbolSize = symbol->st_size;
		*_symbolType = symbol->Type() == STT_FUNC ? B_SYMBOL_TYPE_TEXT
			: B_SYMBOL_TYPE_DATA;

		return B_OK;
	}
}


size_t
SymbolTableBasedImage::_SymbolNameLen(const char* symbolName) const
{
	if (symbolName == NULL || (addr_t)symbolName < (addr_t)fStringTable
		|| (addr_t)symbolName >= (addr_t)fStringTable + fStringTableSize) {
		return 0;
	}

	return strnlen(symbolName,
		(addr_t)fStringTable + fStringTableSize - (addr_t)symbolName);
}


// #pragma mark - ImageFile


ImageFile::ImageFile()
	:
	fFD(-1),
	fFileSize(0),
	fMappedFile((uint8*)MAP_FAILED)
{
}


ImageFile::~ImageFile()
{
	if (fMappedFile != MAP_FAILED)
		munmap(fMappedFile, fFileSize);

	if (fFD >= 0)
		close(fFD);
}


status_t
ImageFile::Init(const image_info& info)
{
	// just copy the image info
	fInfo = info;

	// load the file
	addr_t textAddress;
	size_t textSize;
	addr_t dataAddress;
	size_t dataSize;
	status_t error = _LoadFile(info.name, &textAddress, &textSize, &dataAddress,
		&dataSize);
	if (error != B_OK)
		return error;

	// compute the load delta
	fLoadDelta = (addr_t)fInfo.text - textAddress;

	return B_OK;
}


status_t
ImageFile::Init(const char* path)
{
	// load the file
	addr_t textAddress;
	size_t textSize;
	addr_t dataAddress;
	size_t dataSize;
	status_t error = _LoadFile(path, &textAddress, &textSize, &dataAddress,
		&dataSize);
	if (error != B_OK)
		return error;

	// init the image info
	fInfo.id = -1;
	fInfo.type = B_LIBRARY_IMAGE;
	fInfo.sequence = 0;
	fInfo.init_order = 0;
	fInfo.init_routine = 0;
	fInfo.term_routine = 0;
	fInfo.device = -1;
	fInfo.node = -1;
	strlcpy(fInfo.name, path, sizeof(fInfo.name));
	fInfo.text = (void*)textAddress;
	fInfo.data = (void*)dataAddress;
	fInfo.text_size = textSize;
	fInfo.data_size = dataSize;

	// the image isn't loaded, so no delta
	fLoadDelta = 0;

	return B_OK;
}


status_t
ImageFile::_LoadFile(const char* path, addr_t* _textAddress, size_t* _textSize,
	addr_t* _dataAddress, size_t* _dataSize)
{
	// open and stat() the file
	fFD = open(path, O_RDONLY);
	if (fFD < 0)
		return errno;

	struct stat st;
	if (fstat(fFD, &st) < 0)
		return errno;

	fFileSize = st.st_size;
	if (fFileSize < (off_t)sizeof(elf_ehdr))
		return B_NOT_AN_EXECUTABLE;

	// map it
	fMappedFile = (uint8*)mmap(NULL, fFileSize, PROT_READ, MAP_PRIVATE, fFD, 0);
	if (fMappedFile == MAP_FAILED)
		return errno;

	// examine the elf header
	elf_ehdr* elfHeader = (elf_ehdr*)fMappedFile;
	if (memcmp(elfHeader->e_ident, ELF_MAGIC, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (elfHeader->e_ident[4] != ELFCLASS32)
		return B_NOT_AN_EXECUTABLE;

	// verify the location of the program headers
	int32 programHeaderCount = elfHeader->e_phnum;
	if (elfHeader->e_phoff < sizeof(elf_ehdr)
		|| elfHeader->e_phentsize < sizeof(elf_phdr)
		|| (off_t)(elfHeader->e_phoff + programHeaderCount
				* elfHeader->e_phentsize)
			> fFileSize) {
		return B_NOT_AN_EXECUTABLE;
	}

	elf_phdr* programHeaders
		= (elf_phdr*)(fMappedFile + elfHeader->e_phoff);

	// verify the location of the section headers
	int32 sectionCount = elfHeader->e_shnum;
	if (elfHeader->e_shoff < sizeof(elf_ehdr)
		|| elfHeader->e_shentsize < sizeof(elf_shdr)
		|| (off_t)(elfHeader->e_shoff + sectionCount * elfHeader->e_shentsize)
			> fFileSize) {
		return B_NOT_AN_EXECUTABLE;
	}

	elf_shdr* sectionHeaders
		= (elf_shdr*)(fMappedFile + elfHeader->e_shoff);

	// find the text and data segment -- we need load address and size
	*_textAddress = 0;
	*_textSize = 0;
	*_dataAddress = 0;
	*_dataSize = 0;
	for (int32 i = 0; i < programHeaderCount; i++) {
		elf_phdr* header = (elf_phdr*)
			((uint8*)programHeaders + i * elfHeader->e_phentsize);
		if (header->p_type == PT_LOAD) {
			if ((header->p_flags & PF_WRITE) == 0) {
				*_textAddress = header->p_vaddr;
				*_textSize = header->p_memsz;
			} else {
				*_dataAddress = header->p_vaddr;
				*_dataSize = header->p_memsz;
				break;
			}
		}
	}

	// find the symbol table
	for (int32 i = 0; i < elfHeader->e_shnum; i++) {
		elf_shdr* sectionHeader = (elf_shdr*)
			((uint8*)sectionHeaders + i * elfHeader->e_shentsize);

		if (sectionHeader->sh_type == SHT_SYMTAB) {
			elf_shdr& stringHeader = *(elf_shdr*)
				((uint8*)sectionHeaders
					+ sectionHeader->sh_link * elfHeader->e_shentsize);

			if (stringHeader.sh_type != SHT_STRTAB)
				return B_BAD_DATA;

			if ((off_t)(sectionHeader->sh_offset + sectionHeader->sh_size)
					> fFileSize
				|| (off_t)(stringHeader.sh_offset + stringHeader.sh_size)
					> fFileSize) {
				return B_BAD_DATA;
			}

			fSymbolTable = (elf_sym*)(fMappedFile + sectionHeader->sh_offset);
			fStringTable = (char*)(fMappedFile + stringHeader.sh_offset);
			fSymbolCount = sectionHeader->sh_size / sizeof(elf_sym);
			fStringTableSize = stringHeader.sh_size;

			return B_OK;
		}
	}

	return B_BAD_DATA;
}


// #pragma mark - KernelImage


KernelImage::KernelImage()
{
}


KernelImage::~KernelImage()
{
	delete[] fSymbolTable;
	delete[] fStringTable;
}


status_t
KernelImage::Init(const image_info& info)
{
	fInfo = info;

	// get the table sizes
	fSymbolCount = 0;
	fStringTableSize = 0;
	status_t error = _kern_read_kernel_image_symbols(fInfo.id,
		NULL, &fSymbolCount, NULL, &fStringTableSize, NULL);
	if (error != B_OK)
		return error;

	// allocate the tables
	fSymbolTable = new(std::nothrow) elf_sym[fSymbolCount];
	fStringTable = new(std::nothrow) char[fStringTableSize];
	if (fSymbolTable == NULL || fStringTable == NULL)
		return B_NO_MEMORY;

	// get the info
	return _kern_read_kernel_image_symbols(fInfo.id,
		fSymbolTable, &fSymbolCount, fStringTable, &fStringTableSize,
		&fLoadDelta);
}
