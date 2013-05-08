/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <MergedDirectory.h>

#include <new>
#include <set>
#include <string>

#include <Directory.h>
#include <Entry.h>

#include <AutoDeleter.h>

#include "storage_support.h"


struct BMergedDirectory::EntryNameSet : std::set<std::string> {
};


BMergedDirectory::BMergedDirectory(BPolicy policy)
	:
	BEntryList(),
	fDirectories(10, true),
	fPolicy(policy),
	fDirectoryIndex(0),
	fVisitedEntries(NULL)
{
}


BMergedDirectory::~BMergedDirectory()
{
	delete fVisitedEntries;
}


status_t
BMergedDirectory::Init()
{
	delete fVisitedEntries;
	fDirectories.MakeEmpty(true);

	fVisitedEntries = new(std::nothrow) EntryNameSet;
	return fVisitedEntries != NULL ? B_OK : B_NO_MEMORY;
}


BMergedDirectory::BPolicy
BMergedDirectory::Policy() const
{
	return fPolicy;
}


void
BMergedDirectory::SetPolicy(BPolicy policy)
{
	fPolicy = policy;
}


status_t
BMergedDirectory::AddDirectory(BDirectory* directory)
{
	return fDirectories.AddItem(directory) ? B_OK : B_NO_MEMORY;
}


status_t
BMergedDirectory::AddDirectory(const char* path)
{
	BDirectory* directory = new(std::nothrow) BDirectory(path);
	if (directory == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BDirectory> directoryDeleter(directory);

	if (directory->InitCheck() != B_OK)
		return directory->InitCheck();

	status_t error = AddDirectory(directory);
	if (error != B_OK)
		return error;
	directoryDeleter.Detach();

	return B_OK;
}


status_t
BMergedDirectory::GetNextEntry(BEntry* entry, bool traverse)
{
	entry_ref ref;
	status_t error = GetNextRef(&ref);
	if (error != B_OK)
		return error;

	return entry->SetTo(&ref, traverse);
}


status_t
BMergedDirectory::GetNextRef(entry_ref* ref)
{
	BPrivate::Storage::LongDirEntry dirEntry;
	int32 result = GetNextDirents(&dirEntry, sizeof(dirEntry), 1);
	if (result < 0)
		return result;
	if (result == 0)
		return B_ENTRY_NOT_FOUND;

	BEntry entry;
	status_t error
		= entry.SetTo(fDirectories.ItemAt(fDirectoryIndex), dirEntry.d_name);
	if (error != B_OK)
		return error;

	return entry.GetRef(ref);
}


int32
BMergedDirectory::GetNextDirents(struct dirent* direntBuffer, size_t bufferSize,
	int32 maxEntries)
{
	if (maxEntries <= 0)
		return B_BAD_VALUE;

	while (fDirectoryIndex < fDirectories.CountItems()) {
		int32 count = fDirectories.ItemAt(fDirectoryIndex)->GetNextDirents(
			direntBuffer, bufferSize, 1);
		if (count < 0)
			return count;
		if (count == 0) {
			fDirectoryIndex++;
			continue;
		}

		if (strcmp(direntBuffer->d_name, ".") == 0
			|| strcmp(direntBuffer->d_name, "..") == 0) {
			continue;
		}

		switch (fPolicy) {
			case B_ALLOW_DUPLICATES:
				return count;

			case B_ALWAYS_FIRST:
			case B_COMPARE:
				if (fVisitedEntries != NULL
					&& fVisitedEntries->find(direntBuffer->d_name)
						!= fVisitedEntries->end()) {
					continue;
				}

				if (fVisitedEntries != NULL) {
					try {
						fVisitedEntries->insert(direntBuffer->d_name);
					} catch (std::bad_alloc&) {
						return B_NO_MEMORY;
					}
				}

				if (fPolicy == B_ALWAYS_FIRST
					|| _IsBestEntry(direntBuffer->d_name)) {
					return 1;
				}

				fVisitedEntries->erase(direntBuffer->d_name);
				break;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
BMergedDirectory::Rewind()
{
	for (int32 i = 0; BDirectory* directory = fDirectories.ItemAt(i); i++)
		directory->Rewind();

	if (fVisitedEntries != NULL)
		fVisitedEntries->clear();

	fDirectoryIndex = 0;
	return B_OK;
}


int32
BMergedDirectory::CountEntries()
{
	int32 count = 0;
	char buffer[sizeof(dirent) + B_FILE_NAME_LENGTH];
	while (GetNextDirents((dirent*)&buffer, sizeof(buffer), 1) == 1)
		count++;
	return count;
}


bool
BMergedDirectory::ShallPreferFirstEntry(const entry_ref& entry1, int32 index1,
	const entry_ref& entry2, int32 index2)
{
	// That's basically B_ALWAYS_FIRST semantics. A derived class will implement
	// the desired semantics.
	return true;
}


bool
BMergedDirectory::_IsBestEntry(const char* name)
{
	entry_ref bestEntry;
	if (BEntry(fDirectories.ItemAt(fDirectoryIndex), name).GetRef(&bestEntry)
			!= B_OK) {
		return false;
	}

	int32 directoryCount = fDirectories.CountItems();
	for (int32 i = fDirectoryIndex + 1; i < directoryCount; i++) {
		BEntry entry(fDirectories.ItemAt(i), name);
		entry_ref ref;
		if (entry.Exists() && entry.GetRef(&ref) == B_OK
			&& !ShallPreferFirstEntry(bestEntry, fDirectoryIndex, ref, i)) {
			return false;
		}
	}

	return true;
}
