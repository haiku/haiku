/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "LocatableFile.h"

#include <AutoLocker.h>

#include "LocatableDirectory.h"


// #pragma mark - LocatableFile


LocatableFile::LocatableFile(LocatableEntryOwner* owner,
	LocatableDirectory* directory, const BString& name)
	:
	LocatableEntry(owner, directory),
	fName(name),
	fLocatedPath(),
	fListeners(8)
{
}


LocatableFile::~LocatableFile()
{
}


const char*
LocatableFile::Name() const
{
	return fName.String();
}


void
LocatableFile::GetPath(BString& _path) const
{
	fParent->GetPath(_path);
	_path << '/' << fName;
}


bool
LocatableFile::GetLocatedPath(BString& _path) const
{
	AutoLocker<LocatableEntryOwner> locker(fOwner);

	if (fLocatedPath.Length() > 0) {
		_path = fLocatedPath;
		return true;
	}

	if (!fParent->GetLocatedPath(_path))
		return false;

	_path << '/' << fName;
	return true;
}


void
LocatableFile::SetLocatedPath(const BString& path, bool implicit)
{
	// called with owner already locked

	if (implicit) {
		fLocatedPath = (const char*)NULL;
		fState = LOCATABLE_ENTRY_LOCATED_IMPLICITLY;
	} else {
		fLocatedPath = path;
		fState = LOCATABLE_ENTRY_LOCATED_EXPLICITLY;
	}

	_NotifyListeners();
}


bool
LocatableFile::AddListener(Listener* listener)
{
	AutoLocker<LocatableEntryOwner> locker(fOwner);
	return fListeners.AddItem(listener);
}


void
LocatableFile::RemoveListener(Listener* listener)
{
	AutoLocker<LocatableEntryOwner> locker(fOwner);
	fListeners.RemoveItem(listener);
}


void
LocatableFile::_NotifyListeners()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		fListeners.ItemAt(i)->LocatableFileChanged(this);
}


// #pragma mark - Listener


LocatableFile::Listener::~Listener()
{
}
