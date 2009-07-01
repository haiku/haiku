/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "LocatableDirectory.h"


LocatableDirectory::LocatableDirectory(LocatableEntryOwner* owner,
	LocatableDirectory* parent, const BString& path)
	:
	LocatableEntry(owner, parent),
	fPath(path),
	fLocatedPath()
{
}


LocatableDirectory::~LocatableDirectory()
{
}


const char*
LocatableDirectory::Name() const
{
	int32 lastSlash = fPath.FindLast('/');
		// return -1, if not found
	return fPath.String() + (lastSlash + 1);
}


const char*
LocatableDirectory::Path() const
{
	return fPath.String();
}


void
LocatableDirectory::GetPath(BString& _path) const
{
	_path = fPath;
}


bool
LocatableDirectory::GetLocatedPath(BString& _path) const
{
	if (fLocatedPath.Length() == 0)
		return false;
	_path = fLocatedPath;
	return true;
}


void
LocatableDirectory::SetLocatedPath(const BString& path, bool implicit)
{
	fLocatedPath = path;
	fState = implicit
		? LOCATABLE_ENTRY_LOCATED_IMPLICITLY
		: LOCATABLE_ENTRY_LOCATED_EXPLICITLY;
}


void
LocatableDirectory::AddEntry(LocatableEntry* entry)
{
	fEntries.Add(entry);
}


void
LocatableDirectory::RemoveEntry(LocatableEntry* entry)
{
	fEntries.Remove(entry);
}
