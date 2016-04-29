/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ElfFile.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "ElfSymbolLookup.h"
#include "Tracing.h"


// #pragma mark - ElfSection


ElfSection::ElfSection(const char* name, int fd, uint64 offset, uint64 size,
	target_addr_t loadAddress, uint32 flags)
	:
	fName(name),
	fFD(fd),
	fOffset(offset),
	fSize(size),
	fData(NULL),
	fLoadAddress(loadAddress),
	fFlags(flags),
	fLoadCount(0)
{
}


ElfSection::~ElfSection()
{
	free(fData);
}


status_t
ElfSection::Load()
{
	if (fLoadCount > 0) {
		fLoadCount++;
		return B_OK;
	}

	fData = malloc(fSize);
	if (fData == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = pread(fFD, fData, fSize, fOffset);
	if (bytesRead < 0 || (uint64)bytesRead != fSize) {
		free(fData);
		fData = NULL;
		return bytesRead < 0 ? errno : B_ERROR;
	}

	fLoadCount++;
	return B_OK;
}


void
ElfSection::Unload()
{
	if (fLoadCount == 0)
		return;

	if (--fLoadCount == 0) {
		free(fData);
		fData = NULL;
	}
}


// #pragma mark - ElfSegment


ElfSegment::ElfSegment(uint32 type, uint64 fileOffset, uint64 fileSize,
	target_addr_t loadAddress, target_size_t loadSize, uint32 flags)
	:
	fFileOffset(fileOffset),
	fFileSize(fileSize),
	fLoadAddress(loadAddress),
	fLoadSize(loadSize),
	fType(type),
	fFlags(flags)
{
}


ElfSegment::~ElfSegment()
{
}


// #pragma mark - SymbolLookupSource


struct ElfFile::SymbolLookupSource : public ElfSymbolLookupSource {
	SymbolLookupSource(int fd, uint64 fileOffset, uint64 fileLength,
		uint64 memoryAddress)
		:
		fFd(fd),
		fFileOffset(fileOffset),
		fFileLength(fileLength),
		fMemoryAddress(memoryAddress)
	{
	}

	virtual ssize_t Read(uint64 address, void* buffer, size_t size)
	{
		if (address < fMemoryAddress || address - fMemoryAddress > fFileLength)
			return B_BAD_VALUE;

		uint64 offset = address - fMemoryAddress;
		size_t toRead = (size_t)std::min((uint64)size, fFileLength - offset);
		if (toRead == 0)
			return 0;

		ssize_t bytesRead = pread(fFd, buffer, toRead,
			(off_t)(fFileOffset + offset));
		if (bytesRead < 0)
			return errno;
		return bytesRead;
	}

private:
	int		fFd;
	uint64	fFileOffset;
	uint64	fFileLength;
	uint64	fMemoryAddress;
};


// #pragma mark - ElfFile


ElfFile::ElfFile()
	:
	fFileSize(0),
	fFD(-1),
	fType(ET_NONE),
	fMachine(EM_NONE),
	f64Bit(false),
	fSwappedByteOrder(false),
	fSections(16, true),
	fSegments(16, true)
{
}


ElfFile::~ElfFile()
{
	if (fFD >= 0)
		close(fFD);
}


status_t
ElfFile::Init(const char* fileName)
{
	// open file
	fFD = open(fileName, O_RDONLY);
	if (fFD < 0) {
		WARNING("Failed to open \"%s\": %s\n", fileName, strerror(errno));
		return errno;
	}

	// stat() file to get its size
	struct stat st;
	if (fstat(fFD, &st) < 0) {
		WARNING("Failed to stat \"%s\": %s\n", fileName, strerror(errno));
		return errno;
	}
	fFileSize = st.st_size;

	// Read the identification information to determine whether this is an
	// ELF file at all and some relevant properties for reading it.
	uint8 elfIdent[EI_NIDENT];
	ssize_t bytesRead = pread(fFD, elfIdent, sizeof(elfIdent), 0);
	if (bytesRead != (ssize_t)sizeof(elfIdent))
		return bytesRead < 0 ? errno : B_ERROR;

	// magic
	if (!memcmp(elfIdent, ELF_MAGIC, 4) == 0)
		return B_ERROR;

	// endianess
	if (elfIdent[EI_DATA] == ELFDATA2LSB) {
		fSwappedByteOrder = B_HOST_IS_BENDIAN != 0;
	} else if (elfIdent[EI_DATA] == ELFDATA2MSB) {
		fSwappedByteOrder = B_HOST_IS_LENDIAN != 0;
	} else {
		WARNING("%s: Invalid ELF data byte order: %d\n", fileName,
			elfIdent[EI_DATA]);
		return B_BAD_DATA;
	}

	// determine class and load
	if(elfIdent[EI_CLASS] == ELFCLASS64) {
		f64Bit = true;
		return _LoadFile<ElfClass64>(fileName);
	}
	if(elfIdent[EI_CLASS] == ELFCLASS32) {
		f64Bit = false;
		return _LoadFile<ElfClass32>(fileName);
	}

	WARNING("%s: Invalid ELF class: %d\n", fileName, elfIdent[EI_CLASS]);
	return B_BAD_DATA;
}


ElfSection*
ElfFile::GetSection(const char* name)
{
	ElfSection* section = FindSection(name);
	if (section != NULL && section->Load() == B_OK)
		return section;

	return NULL;
}


void
ElfFile::PutSection(ElfSection* section)
{
	if (section != NULL)
		section->Unload();
}


ElfSection*
ElfFile::FindSection(const char* name) const
{
	int32 count = fSections.CountItems();
	for (int32 i = 0; i < count; i++) {
		ElfSection* section = fSections.ItemAt(i);
		if (strcmp(section->Name(), name) == 0)
			return section;
	}

	return NULL;
}


ElfSegment*
ElfFile::TextSegment() const
{
	int32 count = fSegments.CountItems();
	for (int32 i = 0; i < count; i++) {
		ElfSegment* segment = fSegments.ItemAt(i);
		if (segment->Type() == PT_LOAD && !segment->IsWritable())
			return segment;
	}

	return NULL;
}


ElfSegment*
ElfFile::DataSegment() const
{
	int32 count = fSegments.CountItems();
	for (int32 i = 0; i < count; i++) {
		ElfSegment* segment = fSegments.ItemAt(i);
		if (segment->Type() == PT_LOAD && segment->IsWritable())
			return segment;
	}

	return NULL;
}


ElfSymbolLookupSource*
ElfFile::CreateSymbolLookupSource(uint64 fileOffset, uint64 fileLength,
	uint64 memoryAddress) const
{
	return new(std::nothrow) SymbolLookupSource(fFD, fileOffset, fileLength,
		memoryAddress);
}


template<typename ElfClass>
status_t
ElfFile::_LoadFile(const char* fileName)
{
	typedef typename ElfClass::Ehdr Ehdr;
	typedef typename ElfClass::Phdr Phdr;
	typedef typename ElfClass::Shdr Shdr;

	// read the elf header
	Ehdr elfHeader;
	ssize_t bytesRead = pread(fFD, &elfHeader, sizeof(elfHeader), 0);
	if (bytesRead != (ssize_t)sizeof(elfHeader))
		return bytesRead < 0 ? errno : B_ERROR;

	// check the ELF header
	if (!_CheckRange(0, sizeof(elfHeader))
		|| !_CheckElfHeader<ElfClass>(elfHeader)) {
		WARNING("\"%s\": Not a valid ELF file\n", fileName);
		return B_BAD_DATA;
	}

	fType = Get(elfHeader.e_type);
	fMachine = Get(elfHeader.e_machine);

	if (Get(elfHeader.e_shnum) > 0) {
		// check section header table values
		uint64 sectionHeadersOffset = Get(elfHeader.e_shoff);
		size_t sectionHeaderSize = Get(elfHeader.e_shentsize);
		int sectionCount = Get(elfHeader.e_shnum);
		size_t sectionHeaderTableSize = sectionHeaderSize * sectionCount;
		if (!_CheckRange(sectionHeadersOffset, sectionHeaderTableSize)) {
			WARNING("\"%s\": Invalid ELF header\n", fileName);
			return B_BAD_DATA;
		}

		// read the section header table
		uint8* sectionHeaderTable = (uint8*)malloc(sectionHeaderTableSize);
		if (sectionHeaderTable == NULL)
			return B_NO_MEMORY;
		MemoryDeleter sectionHeaderTableDeleter(sectionHeaderTable);

		bytesRead = pread(fFD, sectionHeaderTable, sectionHeaderTableSize,
			sectionHeadersOffset);
		if (bytesRead != (ssize_t)sectionHeaderTableSize)
			return bytesRead < 0 ? errno : B_ERROR;

		// check and get the section header string section
		Shdr* stringSectionHeader = (Shdr*)(sectionHeaderTable
			+ Get(elfHeader.e_shstrndx) * sectionHeaderSize);
		if (!_CheckRange(Get(stringSectionHeader->sh_offset),
				Get(stringSectionHeader->sh_size))) {
			WARNING("\"%s\": Invalid string section header\n", fileName);
			return B_BAD_DATA;
		}
		size_t sectionStringSize = Get(stringSectionHeader->sh_size);

		ElfSection* sectionStringSection = new(std::nothrow) ElfSection(
			".shstrtab", fFD, Get(stringSectionHeader->sh_offset),
			sectionStringSize, Get(stringSectionHeader->sh_addr),
			Get(stringSectionHeader->sh_flags));
		if (sectionStringSection == NULL)
			return B_NO_MEMORY;
		if (!fSections.AddItem(sectionStringSection)) {
			delete sectionStringSection;
			return B_NO_MEMORY;
		}

		status_t error = sectionStringSection->Load();
		if (error != B_OK)
			return error;

		const char* sectionStrings = (const char*)sectionStringSection->Data();

		// read the other sections
		for (int i = 0; i < sectionCount; i++) {
			Shdr* sectionHeader = (Shdr*)(sectionHeaderTable + i
				* sectionHeaderSize);
			// skip invalid sections and the section header string section
			const char* name = sectionStrings + Get(sectionHeader->sh_name);
			if (Get(sectionHeader->sh_name) >= sectionStringSize
				|| !_CheckRange(Get(sectionHeader->sh_offset),
					Get(sectionHeader->sh_size))
				|| i == Get(elfHeader.e_shstrndx)) {
				continue;
			}

			// create an ElfSection
			ElfSection* section = new(std::nothrow) ElfSection(name, fFD,
				Get(sectionHeader->sh_offset), Get(sectionHeader->sh_size),
				Get(sectionHeader->sh_addr), Get(sectionHeader->sh_flags));
			if (section == NULL)
				return B_NO_MEMORY;
			if (!fSections.AddItem(section)) {
				delete section;
				return B_NO_MEMORY;
			}
		}
	}

	if (Get(elfHeader.e_phnum) > 0) {
		// check program header table values
		uint64 programHeadersOffset = Get(elfHeader.e_phoff);
		size_t programHeaderSize = Get(elfHeader.e_phentsize);
		int segmentCount = Get(elfHeader.e_phnum);
		size_t programHeaderTableSize = programHeaderSize * segmentCount;
		if (!_CheckRange(programHeadersOffset, programHeaderTableSize)) {
			WARNING("\"%s\": Invalid ELF header\n", fileName);
			return B_BAD_DATA;
		}

		// read the program header table
		uint8* programHeaderTable = (uint8*)malloc(programHeaderTableSize);
		if (programHeaderTable == NULL)
			return B_NO_MEMORY;
		MemoryDeleter programHeaderTableDeleter(programHeaderTable);

		bytesRead = pread(fFD, programHeaderTable, programHeaderTableSize,
			programHeadersOffset);
		if (bytesRead != (ssize_t)programHeaderTableSize)
			return bytesRead < 0 ? errno : B_ERROR;

		// read the program headers and create ElfSegment objects
		for (int i = 0; i < segmentCount; i++) {
			Phdr* programHeader = (Phdr*)(programHeaderTable + i
				* programHeaderSize);
			// skip invalid program headers
			if (Get(programHeader->p_filesz) > 0
				&& !_CheckRange(Get(programHeader->p_offset),
					Get(programHeader->p_filesz))) {
				continue;
			}

			// create an ElfSegment
			ElfSegment* segment = new(std::nothrow) ElfSegment(
				Get(programHeader->p_type), Get(programHeader->p_offset),
				Get(programHeader->p_filesz), Get(programHeader->p_vaddr),
				Get(programHeader->p_memsz), Get(programHeader->p_flags));
			if (segment == NULL)
				return B_NO_MEMORY;
			if (!fSegments.AddItem(segment)) {
				delete segment;
				return B_NO_MEMORY;
			}
		}
	}

	return B_OK;
}


bool
ElfFile::_CheckRange(uint64 offset, uint64 size) const
{
	return offset < fFileSize && offset + size <= fFileSize;
}


template<typename ElfClass>
bool
ElfFile::_CheckElfHeader(typename ElfClass::Ehdr& elfHeader)
{
	if (Get(elfHeader.e_shnum) > 0) {
		if (Get(elfHeader.e_shoff) == 0
			|| Get(elfHeader.e_shentsize) < sizeof(typename ElfClass::Shdr)
			|| Get(elfHeader.e_shstrndx) == SHN_UNDEF
			|| Get(elfHeader.e_shstrndx) >= Get(elfHeader.e_shnum)) {
			return false;
		}
	}

	if (Get(elfHeader.e_phnum) > 0) {
		if (Get(elfHeader.e_phoff) == 0
			|| Get(elfHeader.e_phentsize) < sizeof(typename ElfClass::Phdr)) {
			return false;
		}
	}

	return true;
}
