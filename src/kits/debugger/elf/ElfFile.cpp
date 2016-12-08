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


ElfSection::ElfSection(const char* name, uint32 type, int fd, uint64 offset,
	uint64 size, target_addr_t loadAddress, uint32 flags, uint32 linkIndex)
	:
	fName(name),
	fType(type),
	fFD(fd),
	fOffset(offset),
	fSize(size),
	fData(NULL),
	fLoadAddress(loadAddress),
	fFlags(flags),
	fLoadCount(0),
	fLinkIndex(linkIndex)
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
	SymbolLookupSource(int fd)
		:
		fFd(fd),
		fSegments(8, true)
	{
	}

	bool AddSegment(uint64 fileOffset, uint64 fileLength, uint64 memoryAddress)
	{
		Segment* segment = new(std::nothrow) Segment(fileOffset, fileLength,
			memoryAddress);
		if (segment == NULL || !fSegments.AddItem(segment)) {
			delete segment;
			return false;
		}
		return true;
	}

	virtual ssize_t Read(uint64 address, void* buffer, size_t size)
	{
		for (int32 i = 0; Segment* segment = fSegments.ItemAt(i); i++) {
			if (address < segment->fMemoryAddress
					|| address - segment->fMemoryAddress
						> segment->fFileLength) {
				continue;
			}

			uint64 offset = address - segment->fMemoryAddress;
			size_t toRead = (size_t)std::min((uint64)size,
				segment->fFileLength - offset);
			if (toRead == 0)
				return 0;

			ssize_t bytesRead = pread(fFd, buffer, toRead,
				(off_t)(segment->fFileOffset + offset));
			if (bytesRead < 0)
				return errno;
			return bytesRead;
		}

		return B_BAD_VALUE;
	}

private:
	struct Segment {
		uint64	fFileOffset;
		uint64	fFileLength;
		uint64	fMemoryAddress;

		Segment(uint64 fileOffset, uint64 fileLength, uint64 memoryAddress)
			:
			fFileOffset(fileOffset),
			fFileLength(fileLength),
			fMemoryAddress(memoryAddress)
		{
		}
	};

private:
	int						fFd;
	BObjectList<Segment>	fSegments;
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
	if (!memcmp(elfIdent, ELFMAG, 4) == 0)
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


ElfSection*
ElfFile::FindSection(uint32 type) const
{
	int32 count = fSections.CountItems();
	for (int32 i = 0; i < count; i++) {
		ElfSection* section = fSections.ItemAt(i);
		if (section->Type() == type)
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
	SymbolLookupSource* source = new(std::nothrow) SymbolLookupSource(fFD);
	if (source == NULL
			|| !source->AddSegment(fileOffset, fileLength, memoryAddress)) {
		return NULL;
	}

	return source;
}


status_t
ElfFile::CreateSymbolLookup(uint64 textDelta, ElfSymbolLookup*& _lookup) const
{
	// Get the symbol table + corresponding string section. There may be two
	// symbol tables: the dynamic and the non-dynamic one. The former contains
	// only the symbols needed at run-time. The latter, if existing, is likely
	// more complete. So try to find and use the latter one, falling back to the
	// former.
	ElfSection* symbolSection;
	ElfSection* stringSection;
	if (!_FindSymbolSections(symbolSection, stringSection, SHT_SYMTAB)
		&& !_FindSymbolSections(symbolSection, stringSection, SHT_DYNSYM)) {
		return B_ENTRY_NOT_FOUND;
	}

	// create a source with a segment for each section
	SymbolLookupSource* source = new(std::nothrow) SymbolLookupSource(fFD);
	if (source == NULL)
		return B_NO_MEMORY;
	BReference<SymbolLookupSource> sourceReference(source, true);

	if (!source->AddSegment(symbolSection->Offset(), symbolSection->Size(),
				symbolSection->Offset())
		|| !source->AddSegment(stringSection->Offset(), stringSection->Size(),
				stringSection->Offset())) {
		return B_NO_MEMORY;
	}

	// create the lookup
	size_t symbolTableEntrySize = Is64Bit()
		? sizeof(ElfClass64::Sym) : sizeof(ElfClass32::Sym);
	uint32 symbolCount = uint32(symbolSection->Size() / symbolTableEntrySize);

	return ElfSymbolLookup::Create(source, symbolSection->Offset(), 0,
		stringSection->Offset(), symbolCount, symbolTableEntrySize, textDelta,
		f64Bit, fSwappedByteOrder, true, _lookup);
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
			".shstrtab", Get(stringSectionHeader->sh_type),fFD,
			Get(stringSectionHeader->sh_offset), sectionStringSize,
			Get(stringSectionHeader->sh_addr),
			Get(stringSectionHeader->sh_flags),
			Get(stringSectionHeader->sh_link));
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
			ElfSection* section = new(std::nothrow) ElfSection(name,
				Get(sectionHeader->sh_type), fFD, Get(sectionHeader->sh_offset),
				Get(sectionHeader->sh_size), Get(sectionHeader->sh_addr),
				Get(sectionHeader->sh_flags), Get(sectionHeader->sh_link));
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
ElfFile::_FindSymbolSections(ElfSection*& _symbolSection,
	ElfSection*& _stringSection, uint32 type) const
{
	// get the symbol table section
	ElfSection* symbolSection = FindSection(type);
	if (symbolSection == NULL)
		return false;

	// The symbol table section is linked to the corresponding string section.
	ElfSection* stringSection = SectionAt(symbolSection->LinkIndex());
	if (stringSection == NULL || stringSection->Type() != SHT_STRTAB)
		return false;

	_symbolSection = symbolSection;
	_stringSection = stringSection;
	return true;
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
