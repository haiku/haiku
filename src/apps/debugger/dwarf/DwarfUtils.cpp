/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DwarfUtils.h"

#include <String.h>


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
