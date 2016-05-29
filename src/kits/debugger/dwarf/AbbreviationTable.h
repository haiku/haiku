/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABBREVIATION_TABLE_H
#define ABBREVIATION_TABLE_H

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "DataReader.h"
#include "Dwarf.h"


struct AbbreviationTableEntry {
	uint32					code;
	off_t					offset;
	off_t					size;
	AbbreviationTableEntry*	next;

	AbbreviationTableEntry(uint32 code, off_t offset, off_t size)
		:
		code(code),
		offset(offset),
		size(size)
	{
	}
};


struct AbbreviationEntry {
	AbbreviationEntry()
	{
	}

	AbbreviationEntry(uint32 code, const void* data, off_t size)
	{
		SetTo(code, data, size);
	}

	void SetTo(uint32 code, const void* data, off_t size)
	{
		fCode = code;
		fAttributesReader.SetTo(data, size, 4);
			// address size doesn't matter here
		fTag = fAttributesReader.ReadUnsignedLEB128(0);
		fHasChildren = fAttributesReader.Read<uint8>(0);
		fData = fAttributesReader.Data();
		fSize = fAttributesReader.BytesRemaining();
	}

	uint32	Code() const		{ return fCode; }
	uint32	Tag() const			{ return fTag; }
	bool	HasChildren() const	{ return fHasChildren == DW_CHILDREN_yes; }

	bool GetNextAttribute(uint32& name, uint32& form)
	{
		name = fAttributesReader.ReadUnsignedLEB128(0);
		form = fAttributesReader.ReadUnsignedLEB128(0);
		return !fAttributesReader.HasOverflow() && (name != 0 || form != 0);
	}

private:
	uint32		fCode;
	const void*	fData;
	off_t		fSize;
	uint32		fTag;
	uint8		fHasChildren;
	DataReader	fAttributesReader;
};


struct AbbreviationTableHashDefinition {
	typedef uint32					KeyType;
	typedef	AbbreviationTableEntry	ValueType;

	size_t HashKey(uint32 key) const
	{
		return (size_t)key;
	}

	size_t Hash(AbbreviationTableEntry* value) const
	{
		return HashKey(value->code);
	}

	bool Compare(uint32 key, AbbreviationTableEntry* value) const
	{
		return value->code == key;
	}

	AbbreviationTableEntry*& GetLink(AbbreviationTableEntry* value) const
	{
		return value->next;
	}
};


class AbbreviationTable : public DoublyLinkedListLinkImpl<AbbreviationTable> {
public:
								AbbreviationTable(off_t offset);
								~AbbreviationTable();

			status_t			Init(const void* section, off_t sectionSize);

			off_t				Offset() const	{ return fOffset; }

			bool				GetAbbreviationEntry(uint32 code,
									AbbreviationEntry& entry);

private:
			typedef BOpenHashTable<AbbreviationTableHashDefinition> EntryTable;

private:
			status_t			_ParseAbbreviationEntry(
									DataReader& abbrevReader, bool& _nullEntry);

private:
			off_t				fOffset;
			const uint8*		fData;
			off_t				fSize;
			EntryTable			fEntryTable;
};


#endif	// ABBREVIATION_TABLE_H
