/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TypeUnit.h"

#include <new>

#include "DebugInfoEntries.h"


TypeUnit::TypeUnit(off_t headerOffset, off_t contentOffset,
	off_t totalSize, off_t abbreviationOffset, off_t typeOffset,
	uint8 addressSize, uint64 signature, bool isDwarf64)
	:
	BaseUnit(headerOffset, contentOffset, totalSize, abbreviationOffset,
		addressSize, isDwarf64),
	fUnitEntry(NULL),
	fTypeEntry(NULL),
	fSignature(signature),
	fTypeOffset(typeOffset)
{
}


TypeUnit::~TypeUnit()
{
}


void
TypeUnit::SetUnitEntry(DIETypeUnit* entry)
{
	fUnitEntry = entry;
}


DebugInfoEntry*
TypeUnit::TypeEntry() const
{
	return fTypeEntry;
}


void
TypeUnit::SetTypeEntry(DebugInfoEntry* entry)
{
	fTypeEntry = entry;
}


dwarf_unit_kind
TypeUnit::Kind() const
{
	return dwarf_unit_kind_type;
}
