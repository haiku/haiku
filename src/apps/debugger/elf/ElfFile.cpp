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
	fFD(-1),
	fElfHeader(NULL)
{
}


ElfFile::~ElfFile()
{
	while (ElfSegment* segment = fSegments.RemoveHead())
		delete segment;

	while (ElfSection* section = fSections.RemoveHead())
		delete section;

	free(fElfHeader);

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

	// read the elf header
	fElfHeader = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
	if (fElfHeader == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = pread(fFD, fElfHeader, sizeof(Elf32_Ehdr), 0);
	if (bytesRead != (ssize_t)sizeof(Elf32_Ehdr))
		return bytesRead < 0 ? errno : B_ERROR;

	// check the ELF header
	if (!_CheckRange(0, sizeof(Elf32_Ehdr)) || !_CheckElfHeader()) {
		WARNING("\"%s\": Not an ELF file\n", fileName);
		return B_BAD_DATA;
	}

	// check section header table values
	off_t sectionHeadersOffset = fElfHeader->e_shoff;
	size_t sectionHeaderSize = fElfHeader->e_shentsize;
	int sectionCount = fElfHeader->e_shnum;
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
	Elf32_Shdr* stringSectionHeader = (Elf32_Shdr*)(sectionHeaderTable
		+ fElfHeader->e_shstrndx * sectionHeaderSize);
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
		Elf32_Shdr* sectionHeader = (Elf32_Shdr*)(sectionHeaderTable
			+ i * sectionHeaderSize);
		// skip invalid sections and the section header string section
		const char* name = sectionStrings + sectionHeader->sh_name;
		if (sectionHeader->sh_name >= sectionStringSize
			|| !_CheckRange(sectionHeader->sh_offset, sectionHeader->sh_size)
			|| i == fElfHeader->e_shstrndx) {
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
	off_t programHeadersOffset = fElfHeader->e_phoff;
	size_t programHeaderSize = fElfHeader->e_phentsize;
	int segmentCount = fElfHeader->e_phnum;
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
		Elf32_Phdr* programHeader = (Elf32_Phdr*)(programHeaderTable
			+ i * programHeaderSize);
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


bool
ElfFile::_CheckRange(off_t offset, off_t size) const
{
	return offset < fFileSize && offset + size < fFileSize;
}


bool
ElfFile::_CheckElfHeader() const
{
	return memcmp(fElfHeader->e_ident, ELF_MAGIC, 4) == 0
		&& fElfHeader->e_ident[4] == ELFCLASS32
		&& fElfHeader->e_shoff > 0
		&& fElfHeader->e_shnum > 0
		&& fElfHeader->e_shentsize >= sizeof(struct Elf32_Shdr)
		&& fElfHeader->e_shstrndx != SHN_UNDEF
		&& fElfHeader->e_shstrndx < fElfHeader->e_shnum
		&& fElfHeader->e_phoff > 0
		&& fElfHeader->e_phnum > 0
		&& fElfHeader->e_phentsize >= sizeof(struct Elf32_Phdr);
}
