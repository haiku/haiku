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
};


#endif	// DWARF_UTILS_H
