/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <EntryOperationEngineBase.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>


namespace BPrivate {


BEntryOperationEngineBase::Entry::Entry(const char* path)
	:
	fDirectory(NULL),
	fPath(path),
	fEntry(NULL),
	fEntryRef(NULL),
	fDirectoryRef(NULL)
{
}


BEntryOperationEngineBase::Entry::Entry(const BDirectory& directory,
	const char* path)
	:
	fDirectory(&directory),
	fPath(path),
	fEntry(NULL),
	fEntryRef(NULL),
	fDirectoryRef(NULL)
{
}


BEntryOperationEngineBase::Entry::Entry(const BEntry& entry)
	:
	fDirectory(NULL),
	fPath(NULL),
	fEntry(&entry),
	fEntryRef(NULL),
	fDirectoryRef(NULL)
{
}


BEntryOperationEngineBase::Entry::Entry(const entry_ref& entryRef)
	:
	fDirectory(NULL),
	fPath(NULL),
	fEntry(NULL),
	fEntryRef(&entryRef),
	fDirectoryRef(NULL)
{
}


BEntryOperationEngineBase::Entry::Entry(const node_ref& directoryRef,
	const char* path)
	:
	fDirectory(NULL),
	fPath(path),
	fEntry(NULL),
	fEntryRef(NULL),
	fDirectoryRef(&directoryRef)
{
}


BEntryOperationEngineBase::Entry::~Entry()
{
}


status_t
BEntryOperationEngineBase::Entry::GetPath(BPath& buffer, const char*& _path)
	const
{
	status_t error = B_OK;

	if (fEntry != NULL) {
		error = buffer.SetTo(fEntry);
	} else if (fDirectory != NULL) {
		error = buffer.SetTo(fDirectory, fPath);
	} else if (fEntryRef != NULL) {
		error = buffer.SetTo(fEntryRef);
	} else if (fDirectoryRef != NULL) {
		BDirectory directory;
		error = directory.SetTo(fDirectoryRef);
		if (error == B_OK)
			error = buffer.SetTo(&directory, fPath);
	} else if (fPath != NULL) {
		_path = fPath;
		return B_OK;
	}

	if (error != B_OK)
		return error;

	_path = buffer.Path();
	return B_OK;
}


BString
BEntryOperationEngineBase::Entry::Path() const
{
	BPath pathBuffer;
	const char* path;
	if (GetPath(pathBuffer, path) == B_OK)
		return BString(path);
	return BString();
}


status_t
BEntryOperationEngineBase::Entry::GetPathOrName(BString& _path) const
{
	_path.Truncate(0);

	BPath buffer;
	const char* path;
	status_t error = GetPath(buffer, path);
	if (error == B_NO_MEMORY)
		return error;

	if (error == B_OK) {
		_path = path;
	} else if (fEntry != NULL) {
		// GetPath() apparently failed, so just return the entry name.
		_path = fEntry->Name();
	} else if (fDirectory != NULL || fDirectoryRef != NULL) {
		if (fPath != NULL && fPath[0] == '/') {
			// absolute path -- just return it
			_path = fPath;
		} else {
			// get the directory path
			BEntry entry;
			if (fDirectory != NULL) {
				error = fDirectory->GetEntry(&entry);
			} else {
				BDirectory directory;
				error = directory.SetTo(fDirectoryRef);
				if (error == B_OK)
					error = directory.GetEntry(&entry);
			}

			if (error != B_OK || (error = entry.GetPath(&buffer)) != B_OK)
				return error;

			_path = buffer.Path();

			// If we additionally have a relative path, append it.
			if (!_path.IsEmpty() && fPath != NULL) {
				int32 length = _path.Length();
				_path << '/' << fPath;
				if (_path.Length() < length + 2)
					return B_NO_MEMORY;
			}
		}
	} else if (fEntryRef != NULL) {
		// Getting the actual path apparently failed, so just return the entry
		// name.
		_path = fEntryRef->name;
	} else if (fPath != NULL)
		_path = fPath;

	return _path.IsEmpty() ? B_NO_MEMORY : B_OK;
}


BString
BEntryOperationEngineBase::Entry::PathOrName() const
{
	BString path;
	return GetPathOrName(path) == B_OK ? path : BString();
}


} // namespace BPrivate
