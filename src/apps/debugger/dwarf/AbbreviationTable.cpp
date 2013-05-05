/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "AbbreviationTable.h"

#include <stdio.h>

#include <new>


AbbreviationTable::AbbreviationTable(off_t offset)
	:
	fOffset(offset),
	fData(NULL),
	fSize(0)
{
}


AbbreviationTable::~AbbreviationTable()
{
}


status_t
AbbreviationTable::Init(const void* section, off_t sectionSize)
{
	if (fOffset < 0 || fOffset >= sectionSize)
		return B_BAD_DATA;

	fData = (uint8*)section + fOffset;
	fSize = sectionSize - fOffset;
		// That's only the maximum size. Will be adjusted at the end.

	status_t error = fEntryTable.Init();
	if (error != B_OK)
		return error;

	DataReader abbrevReader(fData, fSize, 4);
		// address size doesn't matter here

	while (true) {
		bool nullEntry;
		status_t error = _ParseAbbreviationEntry(abbrevReader, nullEntry);
		if (error != B_OK)
			return error;

		if (nullEntry)
			break;
	}

	fSize -= abbrevReader.BytesRemaining();

	return B_OK;
}


bool
AbbreviationTable::GetAbbreviationEntry(uint32 code, AbbreviationEntry& entry)
{
	AbbreviationTableEntry* tableEntry = fEntryTable.Lookup(code);
	if (tableEntry == NULL)
		return false;

	entry.SetTo(code, fData + tableEntry->offset, tableEntry->size);
	return true;
}


status_t
AbbreviationTable::_ParseAbbreviationEntry(DataReader& abbrevReader,
	bool& _nullEntry)
{
	uint32 code = abbrevReader.ReadUnsignedLEB128(0);
	if (code == 0) {
		if (abbrevReader.HasOverflow()) {
			fprintf(stderr, "Invalid abbreviation table 1!\n");
			return B_BAD_DATA;
		}
		_nullEntry = true;
		return B_OK;
	}

	off_t remaining = abbrevReader.BytesRemaining();

/*	uint32 tag =*/ abbrevReader.ReadUnsignedLEB128(0);
/*	uint8 hasChildren =*/ abbrevReader.Read<uint8>(DW_CHILDREN_no);

//	printf("entry: %lu, tag: %lu, children: %d\n", code, tag,
//		hasChildren);

	// parse attribute specifications
	while (true) {
		uint32 attributeName = abbrevReader.ReadUnsignedLEB128(0);
		uint32 attributeForm = abbrevReader.ReadUnsignedLEB128(0);
		if (abbrevReader.HasOverflow()) {
			fprintf(stderr, "Invalid abbreviation table 2!\n");
			return B_BAD_DATA;
		}

		if (attributeName == 0 && attributeForm == 0)
			break;

//		printf("  attr: name: %lu, form: %lu\n", attributeName,
//			attributeForm);
	}

	// create the entry
	if (fEntryTable.Lookup(code) == NULL) {
		AbbreviationTableEntry* entry = new(std::nothrow)
			AbbreviationTableEntry(code, fSize - remaining,
				remaining - abbrevReader.BytesRemaining());
		if (entry == NULL)
			return B_NO_MEMORY;

		fEntryTable.Insert(entry);
	} else {
		fprintf(stderr, "Duplicate abbreviation table entry %" B_PRIu32 "!\n",
			code);
	}

	_nullEntry = false;
	return B_OK;
}
