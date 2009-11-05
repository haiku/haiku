// ElfFile.cpp
//------------------------------------------------------------------------------
//	Copyright (c) 2003, Ingo Weinhold
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ElfFile.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Implementation of classes for accessing ELF file,
//					or more precisely for iterating through their relocation
//					sections.
//------------------------------------------------------------------------------

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ElfFile.h"

// sanity bounds
static const uint32	kMaxELFHeaderSize			= sizeof(Elf32_Ehdr) + 32;

// read_exactly
static
status_t
read_exactly(BPositionIO &file, off_t position, void *buffer, size_t size,
			 const char *errorMessage = NULL)
{
	status_t error = B_OK;
	ssize_t read = file.ReadAt(position, buffer, size);
	if (read < 0)
		error = read;
	else if ((size_t)read != size)
		error = B_ERROR;
	if (error != B_OK && errorMessage)
		puts(errorMessage);
	return error;
}


// ElfSection

class SymbolPatcher::ElfSection {
public:
								ElfSection();
								~ElfSection();

			void				SetTo(ElfFile* file, Elf32_Shdr* header);
			void				Unset();
			bool				IsInitialized() const	{ return fHeader; }

			ElfFile*			GetFile() const;
			Elf32_Shdr*			GetHeader() const		{ return fHeader; }
			const char*			GetName() const;
			uint8*				GetData() const			{ return fData; }
			size_t				GetSize() const;
			Elf32_Word			GetType() const;
			Elf32_Word			GetLink() const;
			Elf32_Word			GetInfo() const;
			size_t				GetEntrySize() const;
			int32				CountEntries() const;

			status_t			Load();
			void				Unload();

			void				Dump();

private:
			ElfFile*			fFile;
			Elf32_Shdr*			fHeader;
			uint8*				fData;
};

// constructor
ElfSection::ElfSection()
	: fFile(NULL),
	  fHeader(NULL),
	  fData(NULL)
{
}

// destructor
ElfSection::~ElfSection()
{
	Unset();
}

// SetTo
void
ElfSection::SetTo(ElfFile* file, Elf32_Shdr* header)
{
	Unset();
	fFile = file;
	fHeader = header;
}

// Unset
void
ElfSection::Unset()
{
	Unload();
	fFile = NULL;
	fHeader = NULL;
}

// GetFile
ElfFile*
ElfSection::GetFile() const
{
	return fFile;
}

// GetName
const char*
ElfSection::GetName() const
{
	const char* name = NULL;
	if (fHeader && fFile) {
		size_t size = 0;
		const char* nameSection = fFile->GetSectionHeaderStrings(&size);
		if (nameSection && fHeader->sh_name < size)
			name = nameSection + fHeader->sh_name;
	}
	return name;
}

// GetSize
size_t
ElfSection::GetSize() const
{
	return fHeader->sh_size;
}

// GetType
Elf32_Word
ElfSection::GetType() const
{
	return fHeader->sh_type;
}

// GetLink
Elf32_Word
ElfSection::GetLink() const
{
	return fHeader->sh_link;
}

// GetInfo
Elf32_Word
ElfSection::GetInfo() const
{
	return fHeader->sh_info;
}

// GetEntrySize
size_t
ElfSection::GetEntrySize() const
{
	return fHeader->sh_entsize;
}

// CountEntries
int32
ElfSection::CountEntries() const
{
	int32 count = 0;
	if (fHeader) {
		if (GetEntrySize() == 0)
			return 0;
		count = GetSize() / GetEntrySize();
	}
	return count;
}

// Load
status_t
ElfSection::Load()
{
	status_t error = B_ERROR;
	if (fHeader && !fData && fHeader->sh_type != SHT_NULL
		 && fHeader->sh_type != SHT_NOBITS) {
		BFile* file = fFile->GetFile();
		// allocate memory
		fData = new uint8[fHeader->sh_size];
		if (!fData)
			return B_NO_MEMORY;
		// read the data
		error = read_exactly(*file, fHeader->sh_offset, fData,
							 fHeader->sh_size, "Failed to read section!\n");
		if (error != B_OK)
			Unload();
	}
	return error;
}

// Unload
void
ElfSection::Unload()
{
	if (fData) {
		delete[] fData;
		fData = NULL;
	}
}

// Dump
void
ElfSection::Dump()
{
	printf("section %32s: size: %lu\n", GetName(), GetSize());
}


// ElfSymbol

// constructor
ElfSymbol::ElfSymbol(ElfSection* section, int32 index)
	: fSection(section),
	  fIndex(index),
	  fSymbol(NULL)
{
}

// destructor
ElfSymbol::~ElfSymbol()
{
	Unset();
}

// SetTo
void
ElfSymbol::SetTo(ElfSection* section, int32 index)
{
	Unset();
	fSection = section;
	fIndex = index;
}

// Unset
void
ElfSymbol::Unset()
{
	fSection = NULL;
	fIndex = -1;
	fSymbol = NULL;
}

// GetSymbolStruct
const Elf32_Sym*
ElfSymbol::GetSymbolStruct()
{
	Elf32_Sym* symbol = fSymbol;
	if (!symbol && fSection && fSection->GetData()) {
		size_t symbolSize = fSection->GetEntrySize();
		if (symbolSize == 0)
			return NULL;
		int32 symbolCount = fSection->GetSize() / symbolSize;
		if (fIndex >= 0 && fIndex < symbolCount)
			symbol = (Elf32_Sym*)(fSection->GetData() + fIndex * symbolSize);
	}
	return symbol;
}

// GetName
const char*
ElfSymbol::GetName()
{
	const char* name = NULL;
	if (const Elf32_Sym* symbol = GetSymbolStruct()) {
		size_t size = 0;
		const char* data = fSection->GetFile()->GetStringSectionStrings(
			fSection->GetLink(), &size);
		if (data && symbol->st_name < size)
			name = data + symbol->st_name;
	}
	return name;
}

// GetBinding
uint32
ElfSymbol::GetBinding()
{
	uint32 binding = STB_LOCAL;
	if (const Elf32_Sym* symbol = GetSymbolStruct())
		binding = ELF32_ST_BIND(symbol->st_info);
	return binding;
}

// GetType
uint32
ElfSymbol::GetType()
{
	uint32 type = STT_NOTYPE;
	if (const Elf32_Sym* symbol = GetSymbolStruct())
		type = ELF32_ST_TYPE(symbol->st_info);
	return type;
}

// GetTargetSectionIndex
uint32
ElfSymbol::GetTargetSectionIndex()
{
	uint32 index = SHN_UNDEF;
	if (const Elf32_Sym* symbol = GetSymbolStruct())
		index = symbol->st_shndx;
	return index;
}


// ElfRelocation

// constructor
ElfRelocation::ElfRelocation(ElfSection* section, int32 index)
	: fSection(section),
	  fIndex(index),
	  fRelocation(NULL)
{
}

// destructor
ElfRelocation::~ElfRelocation()
{
	Unset();
}

// SetTo
void
ElfRelocation::SetTo(ElfSection* section, int32 index)
{
	Unset();
	fSection = section;
	fIndex = index;
}

// Unset
void
ElfRelocation::Unset()
{
	fSection = NULL;
	fIndex = -1;
	fRelocation = NULL;
}

// GetRelocationStruct
Elf32_Rel*
ElfRelocation::GetRelocationStruct()
{
	Elf32_Rel* relocation = fRelocation;
	if (!relocation && fSection) {
		if (!fSection->GetData()) {
			if (fSection->Load() != B_OK)
				return NULL;
		}
		size_t entrySize = fSection->GetEntrySize();
		if (entrySize == 0 || entrySize < sizeof(Elf32_Rel))
			return NULL;
		int32 entryCount = fSection->GetSize() / entrySize;
		if (fIndex >= 0 && fIndex < entryCount) {
			relocation = (Elf32_Rel*)(fSection->GetData()
						 + fIndex * entrySize);
		}
	}
	return relocation;
}

// GetType
uint32
ElfRelocation::GetType()
{
	uint32 type = R_386_NONE;
	if (Elf32_Rel* relocation = GetRelocationStruct())
		type = ELF32_R_TYPE(relocation->r_info);
	return type;
}

// GetSymbolIndex
uint32
ElfRelocation::GetSymbolIndex()
{
	uint32 index = 0;
	if (Elf32_Rel* relocation = GetRelocationStruct())
		index = ELF32_R_SYM(relocation->r_info);
	return index;
}

// GetOffset
Elf32_Addr
ElfRelocation::GetOffset()
{
	Elf32_Addr offset = 0;
	if (Elf32_Rel* relocation = GetRelocationStruct())
		offset = relocation->r_offset;
	return offset;
}

// GetSymbol
status_t
ElfRelocation::GetSymbol(ElfSymbol* symbol)
{
	status_t error = B_BAD_VALUE;
	if (symbol && fSection) {
		uint32 index = GetSymbolIndex();
		if (ElfSection* symbols
			= fSection->GetFile()->SectionAt(fSection->GetLink(), true)) {
			symbol->SetTo(symbols, index);
			if (symbol->GetSymbolStruct())
				error = B_OK;
		}
	}
	return error;
}


// ElfRelocationIterator

// constructor
ElfRelocationIterator::ElfRelocationIterator(ElfFile* file)
	: fFile(file),
	  fSectionIndex(-1),
	  fEntryIndex(-1)
{
}

// destructor
ElfRelocationIterator::~ElfRelocationIterator()
{
}

// GetNext
bool
ElfRelocationIterator::GetNext(ElfRelocation* relocation)
{
	bool result = false;
	if (fFile && relocation) {
		// set to possible entry
		ElfSection* section = NULL;
		if (fSectionIndex < 0) {
			fSectionIndex = 0;
			fEntryIndex = 0;
			section = _FindNextSection();
		} else {
			fEntryIndex++;
			section = fFile->SectionAt(fSectionIndex);
		}
		// find next valid entry
		while (section && fEntryIndex >= section->CountEntries()) {
			fSectionIndex++;
			section = _FindNextSection();
			fEntryIndex = 0;
		}
		// set result
		if (section) {
			relocation->SetTo(section, fEntryIndex);
			result = true;
		}
	}
	return result;
}

// _FindNextSection
ElfSection*
ElfRelocationIterator::_FindNextSection()
{
	if (fFile) {
		for (; fSectionIndex < fFile->CountSections(); fSectionIndex++) {
			ElfSection* section = fFile->SectionAt(fSectionIndex);
			if (section && section->GetType() == SHT_REL)
				return section;
		}
	}
	return NULL;
}


// ElfFile

// constructor
ElfFile::ElfFile()
	: fFile(),
	  fSectionHeaders(NULL),
	  fSections(NULL),
	  fSectionCount(0),
	  fSectionHeaderSize(0)
{
}

// destructor
ElfFile::~ElfFile()
{
	Unset();
}

// SetTo
status_t
ElfFile::SetTo(const char *filename)
{
	Unset();
	status_t error = _SetTo(filename);
	if (error)
		Unset();
	return error;
}

// Unset
void
ElfFile::Unset()
{
	// delete sections
	if (fSections) {
		delete[] fSections;
		fSections = NULL;
	}
	// delete section headers
	if (fSectionHeaders) {
		delete[] fSectionHeaders;
		fSectionHeaders = NULL;
	}
	fSectionCount = 0;
	fSectionHeaderSize = 0;
	fFile.Unset();
}

// Unload
void
ElfFile::Unload()
{
	for (int i = 0; i < fSectionCount; i++)
		fSections[i].Unload();
}

// GetSectionHeaderStrings
const char*
ElfFile::GetSectionHeaderStrings(size_t* size)
{
	return GetStringSectionStrings(fHeader.e_shstrndx, size);
}

// GetStringSectionStrings
const char*
ElfFile::GetStringSectionStrings(int32 index, size_t* _size)
{
	const char* data = NULL;
	size_t size = 0;
	if (ElfSection* section = SectionAt(index, true)) {
		data = (const char*)section->GetData();
		size = (data ? section->GetSize() : 0);
	}
	// set results
	if (_size)
		*_size = size;
	return data;
}

// SectionAt
ElfSection*
ElfFile::SectionAt(int32 index, bool load)
{
	ElfSection* section = NULL;
	if (fSections && index >= 0 && index < fSectionCount) {
		section = fSections + index;
		if (load && !section->GetData()) {
			if (section->Load() != B_OK) {
				section = NULL;
				printf("Failed to load section %ld\n", index);
			}
		}
	}
	return section;
}

// Dump
void
ElfFile::Dump()
{
	printf("%ld sections\n", fSectionCount);
	for (int i = 0; i < fSectionCount; i++)
		fSections[i].Dump();
}

// _SetTo
status_t
ElfFile::_SetTo(const char *filename)
{
	if (!filename)
		return B_BAD_VALUE;
	// open file
	status_t error = fFile.SetTo(filename, B_READ_ONLY);
	// get the file size
	off_t fileSize = 0;
	error = fFile.GetSize(&fileSize);
	if (error != B_OK) {
		printf("Failed to get file size!\n");
		return error;
	}
	// read ELF header
	error = read_exactly(fFile, 0, &fHeader, sizeof(Elf32_Ehdr),
						 "Failed to read ELF object header!\n");
	if (error != B_OK)
		return error;
	// check the ident field
	// magic
	if (fHeader.e_ident[EI_MAG0] != ELFMAG0
		|| fHeader.e_ident[EI_MAG1] != ELFMAG1
		|| fHeader.e_ident[EI_MAG2] != ELFMAG2
		|| fHeader.e_ident[EI_MAG3] != ELFMAG3) {
		printf("Bad ELF file magic!\n");
		return B_BAD_VALUE;
	}
	// class
	if (fHeader.e_ident[EI_CLASS] != ELFCLASS32) {
		printf("Wrong ELF class!\n");
		return B_BAD_VALUE;
	}
	// check data encoding (endianess)
	if (fHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
		printf("Wrong data encoding!\n");
		return B_BAD_VALUE;
	}
	// version
	if (fHeader.e_ident[EI_VERSION] != EV_CURRENT) {
		printf("Wrong data encoding!\n");
		return B_BAD_VALUE;
	}
	// get the header values
	uint32 headerSize				= fHeader.e_ehsize;
	uint32 sectionHeaderTableOffset	= fHeader.e_shoff;
	uint32 sectionHeaderSize		= fHeader.e_shentsize;
	uint32 sectionHeaderCount		= fHeader.e_shnum;
	// check the sanity of the header values
	// ELF header size
	if (headerSize < sizeof(Elf32_Ehdr) || headerSize > kMaxELFHeaderSize) {
		printf("Invalid ELF header: invalid ELF header size: %lu.",
			   headerSize);
		return B_BAD_VALUE;
	}
	// section header table offset
	if (sectionHeaderTableOffset == 0) {
		printf("ELF file has no section header table!\n");
		return B_BAD_VALUE;
	}
	uint32 sectionHeaderTableSize = 0;
	if (sectionHeaderTableOffset < headerSize
		|| sectionHeaderTableOffset > fileSize) {
		printf("Invalid ELF header: invalid section header table offset: %lu.",
			   sectionHeaderTableOffset);
		return B_BAD_VALUE;
	}
	// section header table offset
	sectionHeaderTableSize = sectionHeaderSize * sectionHeaderCount;
	if (sectionHeaderSize < sizeof(Elf32_Shdr)
		|| sectionHeaderTableOffset + sectionHeaderTableSize > fileSize) {
		printf("Invalid ELF header: section header table exceeds file: %lu.",
			   sectionHeaderTableOffset + sectionHeaderTableSize);
		return B_BAD_VALUE;
	}
	// allocate memory for the section header table and read it
	fSectionHeaders = new(std::nothrow) uint8[sectionHeaderTableSize];
	fSectionCount = sectionHeaderCount;
	fSectionHeaderSize = sectionHeaderSize;
	if (!fSectionHeaders)
		return B_NO_MEMORY;
	error = read_exactly(fFile, sectionHeaderTableOffset, fSectionHeaders,
						 sectionHeaderTableSize,
						 "Failed to read section headers!\n");
	if (error != B_OK)
		return error;
	// allocate memory for the section pointers
	fSections = new(std::nothrow) ElfSection[fSectionCount];
	if (!fSections)
		return B_NO_MEMORY;
	// init the sections
	for (int i = 0; i < fSectionCount; i++)
		fSections[i].SetTo(this, _SectionHeaderAt(i));
	return error;
}

// _SectionHeaderAt
Elf32_Shdr*
ElfFile::_SectionHeaderAt(int32 index)
{
	Elf32_Shdr* header = NULL;
	if (fSectionHeaders && index >= 0 && index < fSectionCount)
		header = (Elf32_Shdr*)(fSectionHeaders + index * fSectionHeaderSize);
	return header;
}

// _LoadSection
status_t
ElfFile::_LoadSection(int32 index)
{
	status_t error = B_OK;
	if (fSections && index >= 0 && index < fSectionCount) {
		ElfSection& section = fSections[index];
		error = section.Load();
	} else
		error = B_BAD_VALUE;
	return error;
}

