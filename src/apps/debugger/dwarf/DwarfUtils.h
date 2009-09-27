/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_UTILS_H
#define DWARF_UTILS_H

#include "DebugInfoEntries.h"


class BString;
class DebugInfoEntry;
class DwarfFile;


class DwarfUtils {
public:
	static	void				GetDIEName(const DebugInfoEntry* entry,
									BString& _name);
	static	void				GetFullDIEName(const DebugInfoEntry* entry,
									BString& _name);
	static	void				GetFullyQualifiedDIEName(
									const DebugInfoEntry* entry,
									BString& _name);

	static	bool				GetDeclarationLocation(DwarfFile* dwarfFile,
									const DebugInfoEntry* entry,
									const char*& _directory,
									const char*& _file,
									int32& _line, int32& _column);

	template<typename EntryType, typename Predicate>
	static	EntryType*			GetDIEByPredicate(EntryType* entry,
									const Predicate& predicate);
};


template<typename EntryType, typename Predicate>
/*static*/ EntryType*
DwarfUtils::GetDIEByPredicate(EntryType* entry, const Predicate& predicate)
{
	if (predicate(entry))
		return entry;

	// try the abstract origin
	if (EntryType* abstractOrigin = dynamic_cast<EntryType*>(
			entry->AbstractOrigin())) {
		entry = abstractOrigin;
		if (predicate(entry))
			return entry;
	}

	// try the specification
	if (EntryType* specification = dynamic_cast<EntryType*>(
			entry->Specification())) {
		entry = specification;
		if (predicate(entry))
			return entry;
	}

	return NULL;
}


#endif	// DWARF_UTILS_H
