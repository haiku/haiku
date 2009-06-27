/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "CompilationUnit.h"


CompilationUnit::CompilationUnit(dwarf_off_t headerOffset,
	dwarf_off_t contentOffset, dwarf_off_t totalSize,
	dwarf_off_t abbreviationOffset)
	:
	fHeaderOffset(headerOffset),
	fContentOffset(contentOffset),
	fTotalSize(totalSize),
	fAbbreviationOffset(abbreviationOffset),
	fAbbreviationTable(NULL),
	fUnitEntry(NULL)
{
}


CompilationUnit::~CompilationUnit()
{
}


void
CompilationUnit::SetAbbreviationTable(AbbreviationTable* abbreviationTable)
{
	fAbbreviationTable = abbreviationTable;
}


status_t
CompilationUnit::AddDebugInfoEntry(DebugInfoEntry* entry, dwarf_off_t offset)
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


int
CompilationUnit::CountEntries() const
{
	return fEntries.Count();
}


void
CompilationUnit::GetEntryAt(int index, DebugInfoEntry*& entry,
	dwarf_off_t& offset) const
{
	entry = fEntries[index];
	offset = fEntryOffsets[index];
}


DebugInfoEntry*
CompilationUnit::EntryForOffset(dwarf_off_t offset) const
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
