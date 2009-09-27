/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CompilationUnit.h"

#include <new>

#include "DebugInfoEntries.h"
#include "TargetAddressRangeList.h"


struct CompilationUnit::File {
	BString		fileName;
	const char*	dirName;


	File(const char* fileName, const char* dirName)
		:
		fileName(fileName),
		dirName(dirName)
	{
	}
};


CompilationUnit::CompilationUnit(off_t headerOffset, off_t contentOffset,
	off_t totalSize, off_t abbreviationOffset, uint8 addressSize,
	bool isDwarf64)
	:
	fHeaderOffset(headerOffset),
	fContentOffset(contentOffset),
	fTotalSize(totalSize),
	fAbbreviationOffset(abbreviationOffset),
	fAbbreviationTable(NULL),
	fUnitEntry(NULL),
	fAddressRanges(NULL),
	fDirectories(10, true),
	fFiles(10, true),
	fLineNumberProgram(addressSize),
	fAddressSize(addressSize),
	fIsDwarf64(isDwarf64)
{
}


CompilationUnit::~CompilationUnit()
{
	SetAddressRanges(NULL);
}


void
CompilationUnit::SetAbbreviationTable(AbbreviationTable* abbreviationTable)
{
	fAbbreviationTable = abbreviationTable;
}


status_t
CompilationUnit::AddDebugInfoEntry(DebugInfoEntry* entry, off_t offset)
{
	if (!fEntries.Add(entry))
		return B_NO_MEMORY;
	if (!fEntryOffsets.Add(offset)) {
		fEntries.Remove(fEntries.Count() - 1);
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
CompilationUnit::SetUnitEntry(DIECompileUnitBase* entry)
{
	fUnitEntry = entry;
}


void
CompilationUnit::SetSourceLanguage(const SourceLanguageInfo* language)
{
	fSourceLanguage = language;
}


void
CompilationUnit::SetAddressRanges(TargetAddressRangeList* ranges)
{
	if (fAddressRanges != NULL)
		fAddressRanges->ReleaseReference();

	fAddressRanges = ranges;

	if (fAddressRanges != NULL)
		fAddressRanges->AcquireReference();
}


target_addr_t
CompilationUnit::AddressRangeBase() const
{
	return fUnitEntry != NULL ? fUnitEntry->LowPC() : 0;
}


int
CompilationUnit::CountEntries() const
{
	return fEntries.Count();
}


void
CompilationUnit::GetEntryAt(int index, DebugInfoEntry*& entry,
	off_t& offset) const
{
	entry = fEntries[index];
	offset = fEntryOffsets[index];
}


DebugInfoEntry*
CompilationUnit::EntryForOffset(off_t offset) const
{
	if (fEntries.IsEmpty())
		return NULL;

	// binary search
	int lower = 0;
	int upper = fEntries.Count() - 1;
	while (lower < upper) {
		int mid = (lower + upper + 1) / 2;
		if (fEntryOffsets[mid] > offset)
			upper = mid - 1;
		else
			lower = mid;
	}

	return fEntryOffsets[lower] == offset ? fEntries[lower] : NULL;
}


bool
CompilationUnit::AddDirectory(const char* directory)
{
	BString* directoryString = new(std::nothrow) BString(directory);
	if (directoryString == NULL || directoryString->Length() == 0
		|| !fDirectories.AddItem(directoryString)) {
		delete directoryString;
		return false;
	}

	return true;
}


int32
CompilationUnit::CountDirectories() const
{
	return fDirectories.CountItems();
}


const char*
CompilationUnit::DirectoryAt(int32 index) const
{
	BString* directory = fDirectories.ItemAt(index);
	return directory != NULL ? directory->String() : NULL;
}


bool
CompilationUnit::AddFile(const char* fileName, int32 dirIndex)
{
	File* file = new(std::nothrow) File(fileName, DirectoryAt(dirIndex));
	if (file == NULL || file->fileName.Length() == 0 || !fFiles.AddItem(file)) {
		delete file;
		return false;
	}

	return true;
}


int32
CompilationUnit::CountFiles() const
{
	return fFiles.CountItems();
}


const char*
CompilationUnit::FileAt(int32 index, const char** _directory) const
{
	if (File* file = fFiles.ItemAt(index)) {
		if (_directory != NULL)
			*_directory = file->dirName;
		return file->fileName.String();
	}

	return NULL;
}
