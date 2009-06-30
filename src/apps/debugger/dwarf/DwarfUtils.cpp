/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfUtils.h"

#include <String.h>

#include "CompilationUnit.h"
#include "DwarfFile.h"


/*static*/ void
DwarfUtils::GetDIEName(const DebugInfoEntry* entry, BString& _name)
{
	// If we don't seem to have a name but a specification, return the
	// specification's name.
	const char* name = entry->Name();
	if (name == NULL) {
		if (DebugInfoEntry* specification = entry->Specification())
			name = specification->Name();
	}

	_name = name;
}


/*static*/ void
DwarfUtils::GetFullDIEName(const DebugInfoEntry* entry, BString& _name)
{
	// If we don't seem to have a name but a specification, return the
	// specification's name.
	const char* name = entry->Name();
	if (name == NULL) {
		if (DebugInfoEntry* specification = entry->Specification()) {
			entry = specification;
			name = specification->Name();
		}
	}

	// TODO: Get template and function parameters!

	_name = name;
}


/*static*/ void
DwarfUtils::GetFullyQualifiedDIEName(const DebugInfoEntry* entry,
	BString& _name)
{
	// If we don't seem to have a name but a specification, return the
	// specification's name.
	if (entry->Name() == NULL) {
		if (DebugInfoEntry* specification = entry->Specification())
			entry = specification;
	}

	_name.Truncate(0);

	// Get the namespace, if any.
	DebugInfoEntry* parent = entry->Parent();
	while (parent != NULL) {
		if (parent->IsNamespace()) {
			GetFullyQualifiedDIEName(parent, _name);
			break;
		}

		parent = parent->Parent();
	}

	BString name;
	GetFullDIEName(entry, name);

	if (name.Length() == 0)
		return;

	if (_name.Length() > 0) {
		_name << "::" << name;
	} else
		_name = name;
}


/*static*/ bool
DwarfUtils::GetDeclarationLocation(DwarfFile* dwarfFile,
	const DebugInfoEntry* entry, const char*& _directory, const char*& _file,
	uint32& _line, uint32& _column)
{
	uint32 file;
	uint32 line;
	uint32 column;
	if (!entry->GetDeclarationLocation(file, line, column))
		return false;

	// if no info yet, try the abstract origin (if any)
	if (file == 0) {
		if (DebugInfoEntry* abstractOrigin = entry->AbstractOrigin()) {
			if (abstractOrigin->GetDeclarationLocation(file, line, column)
					&& file != 0) {
				entry = abstractOrigin;
			}
		}
	}

	// if no info yet, try the specification (if any)
	if (file == 0) {
		if (DebugInfoEntry* specification = entry->Specification()) {
			if (specification->GetDeclarationLocation(file, line, column)
					&& file != 0) {
				entry = specification;
			}
		}
	}

	if (file == 0)
		return false;

	// get the compilation unit
	CompilationUnit* unit = dwarfFile->CompilationUnitForDIE(entry);
	if (unit == NULL)
		return false;

	const char* directoryName;
	const char* fileName = unit->FileAt(file - 1, &directoryName);
	if (fileName == NULL)
		return false;

	_directory = directoryName;
	_file = fileName;
	_line = line;
	_column = column;
	return true;
}
