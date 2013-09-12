/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BaseUnit.h"

#include <new>

#include "DebugInfoEntries.h"


BaseUnit::BaseUnit(off_t headerOffset, off_t contentOffset,
	off_t totalSize, off_t abbreviationOffset, uint8 addressSize,
	bool isDwarf64)
	:
	fHeaderOffset(headerOffset),
	fContentOffset(contentOffset),
	fTotalSize(totalSize),
	fAbbreviationOffset(abbreviationOffset),
	fAbbreviationTable(NULL),
	fAddressSize(addressSize),
	fIsDwarf64(isDwarf64)
{
}


BaseUnit::~BaseUnit()
{
}


void
BaseUnit::SetAbbreviationTable(AbbreviationTable* abbreviationTable)
{
	fAbbreviationTable = abbreviationTable;
}


status_t
BaseUnit::AddDebugInfoEntry(DebugInfoEntry* entry, off_t offset)
{
	if (!fEntries.Add(entry))
		return B_NO_MEMORY;
	if (!fEntryOffsets.Add(offset)) {
		fEntries.Remove(fEntries.Count() - 1);
		return B_NO_MEMORY;
	}

	return B_OK;
}


bool
BaseUnit::ContainsAbsoluteOffset(off_t offset) const
{
	return fHeaderOffset <= offset && fHeaderOffset + fTotalSize > offset;
}


void
BaseUnit::SetSourceLanguage(const SourceLanguageInfo* language)
{
	fSourceLanguage = language;
}


int
BaseUnit::CountEntries() const
{
	return fEntries.Count();
}


void
BaseUnit::GetEntryAt(int index, DebugInfoEntry*& entry,
	off_t& offset) const
{
	entry = fEntries[index];
	offset = fEntryOffsets[index];
}


DebugInfoEntry*
BaseUnit::EntryForOffset(off_t offset) const
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
