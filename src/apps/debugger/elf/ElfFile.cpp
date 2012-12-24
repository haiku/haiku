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

#include <new>

#include <AutoDeleter.h>

#include "Tracing.h"


// #pragma mark - ElfSection


ElfSection::ElfSection(const char* name, int fd, off_t offset, off_t size,
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
	if (bytesRead != fSize) {
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


ElfSegment::ElfSegment(off_t fileOffset, off_t fileSize,
	target_addr_t loadAddress, target_size_t loadSize, bool writable)
	:
	fFileOffset(fileOffset),
	fFileSize(fileSize),
	fLoadAddress(loadAddress),
	fLoadSize(loadSize),
	fWritable(writable)
{
}


ElfSegment::~ElfSegment()
{
}


// #pragma mark - ElfFile


ElfFile::ElfFile()
	:
	fFileSize(0),
	fFD(-1)
{
}


ElfFile::~ElfFile()
{
	while (ElfSegment* segment = fSegments.RemoveHead())
		delete segment;

	while (ElfSection* section = fSections.RemoveHead())
		delete section;

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

	// Read the identification information to determine the class.
	uint8 elfIdent[EI_NIDENT];
	ssize_t bytesRead = pread(fFD, elfIdent, sizeof(elfIdent), 0);
	if (bytesRead != (ssize_t)sizeof(elfIdent))
		return bytesRead < 0 ? errno : B_ERROR;

	if(elfIdent[EI_CLASS] == ELFCLASS64)
		return _LoadFile<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr>(fileName);
	else
		return _LoadFile<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr>(fileName);
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
	for (SectionList::ConstIterator it = fSections.GetIterator();
			ElfSection* section = it.Next();) {
		if (strcmp(section->Name(), name) == 0)
			return section;
	}

	return NULL;
}


ElfSegment*
ElfFile::TextSegment() const
{
	for (SegmentList::ConstIterator it = fSegments.GetIterator();
			ElfSegment* segment = it.Next();) {
		if (!segment->IsWritable())
			return segment;
	}

	return NULL;
}


ElfSegment*
ElfFile::DataSegment() const
{
	for (SegmentList::ConstIterator it = fSegments.GetIterator();
			ElfSegment* segment = it.Next();) {
		if (segment->IsWritable())
			return segment;
	}

	return NULL;
}


template<typename Ehdr, typename Phdr, typename Shdr>
status_t
ElfFile::_LoadFile(const char* fileName)
{
	Ehdr elfHeader;

	// read the elf header
	ssize_t bytesRead = pread(fFD, &elfHeader, sizeof(Ehdr), 0);
	if (bytesRead != (ssize_t)sizeof(Ehdr))
		return bytesRead < 0 ? errno : B_ERROR;

	// check the ELF header
	if (!_CheckRange(0, sizeof(Ehdr)) || !_CheckElfHeader(elfHeader)) {
		WARNING("\"%s\": Not an ELF file\n", fileName);
		return B_BAD_DATA;
	}

	// check section header table values
	off_t sectionHeadersOffset = elfHeader.e_shoff;
	size_t sectionHeaderSize = elfHeader.e_shentsize;
	int sectionCount = elfHeader.e_shnum;
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
		+ elfHeader.e_shstrndx * sectionHeaderSize);
	if (!_CheckRange(stringSectionHeader->sh_offset,
			stringSectionHeader->sh_size)) {
		WARNING("\"%s\": Invalid string section header\n", fileName);
		return B_BAD_DATA;
	}
	size_t sectionStringSize = stringSectionHeader->sh_size;

	ElfSection* sectionStringSection = new(std::nothrow) ElfSection(".shstrtab",
		fFD, stringSectionHeader->sh_offset, sectionStringSize,
		stringSectionHeader->sh_addr, stringSectionHeader->sh_flags);
	if (sectionStringSection == NULL)
		return B_NO_MEMORY;
	fSections.Add(sectionStringSection);

	status_t error = sectionStringSection->Load();
	if (error != B_OK)
		return error;

	const char* sectionStrings = (const char*)sectionStringSection->Data();

	// read the other sections
	for (int i = 0; i < sectionCount; i++) {
		Shdr* sectionHeader = (Shdr*)(sectionHeaderTable + i
			* sectionHeaderSize);
		// skip invalid sections and the section header string section
		const char* name = sectionStrings + sectionHeader->sh_name;
		if (sectionHeader->sh_name >= sectionStringSize
			|| !_CheckRange(sectionHeader->sh_offset, sectionHeader->sh_size)
			|| i == elfHeader.e_shstrndx) {
			continue;
		}

		// create an ElfSection
		ElfSection* section = new(std::nothrow) ElfSection(name, fFD,
			sectionHeader->sh_offset, sectionHeader->sh_size,
			sectionHeader->sh_addr, sectionHeader->sh_flags);
		if (section == NULL)
			return B_NO_MEMORY;
		fSections.Add(section);
	}

	// check program header table values
	off_t programHeadersOffset = elfHeader.e_phoff;
	size_t programHeaderSize = elfHeader.e_phentsize;
	int segmentCount = elfHeader.e_phnum;
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
		// skip program headers we aren't interested in or that are invalid
		if (programHeader->p_type != PT_LOAD || programHeader->p_filesz == 0
			|| programHeader->p_memsz == 0
			|| !_CheckRange(programHeader->p_offset, programHeader->p_filesz)) {
			continue;
		}

		// create an ElfSegment
		ElfSegment* segment = new(std::nothrow) ElfSegment(
			programHeader->p_offset, programHeader->p_filesz,
			programHeader->p_vaddr, programHeader->p_memsz,
			(programHeader->p_flags & PF_WRITE) != 0);
		if (segment == NULL)
			return B_NO_MEMORY;
		fSegments.Add(segment);
	}

	return B_OK;
}


bool
ElfFile::_CheckRange(off_t offset, off_t size) const
{
	return offset < fFileSize && offset + size < fFileSize;
}


bool
ElfFile::_CheckElfHeader(Elf32_Ehdr& elfHeader)
{
	return memcmp(elfHeader.e_ident, ELF_MAGIC, 4) == 0
		&& elfHeader.e_ident[4] == ELFCLASS32
		&& elfHeader.e_shoff > 0
		&& elfHeader.e_shnum > 0
		&& elfHeader.e_shentsize >= sizeof(struct Elf32_Shdr)
		&& elfHeader.e_shstrndx != SHN_UNDEF
		&& elfHeader.e_shstrndx < elfHeader.e_shnum
		&& elfHeader.e_phoff > 0
		&& elfHeader.e_phnum > 0
		&& elfHeader.e_phentsize >= sizeof(struct Elf32_Phdr);
}


bool
ElfFile::_CheckElfHeader(Elf64_Ehdr& elfHeader)
{
	return memcmp(elfHeader.e_ident, ELF_MAGIC, 4) == 0
		&& elfHeader.e_ident[4] == ELFCLASS64
		&& elfHeader.e_shoff > 0
		&& elfHeader.e_shnum > 0
		&& elfHeader.e_shentsize >= sizeof(struct Elf64_Shdr)
		&& elfHeader.e_shstrndx != SHN_UNDEF
		&& elfHeader.e_shstrndx < elfHeader.e_shnum
		&& elfHeader.e_phoff > 0
		&& elfHeader.e_phnum > 0
		&& elfHeader.e_phentsize >= sizeof(struct Elf64_Phdr);
}
