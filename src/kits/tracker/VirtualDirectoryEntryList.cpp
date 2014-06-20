/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include "VirtualDirectoryEntryList.h"

#include <AutoLocker.h>
#include <storage_support.h>

#include "Model.h"
#include "VirtualDirectoryManager.h"


namespace BPrivate {

//	#pragma mark - VirtualDirectoryEntryList


VirtualDirectoryEntryList::VirtualDirectoryEntryList(Model* model)
	:
	EntryListBase(),
	fDefinitionFileRef(),
	fMergedDirectory(BMergedDirectory::B_ALWAYS_FIRST)
{
	VirtualDirectoryManager* manager = VirtualDirectoryManager::Instance();
	if (manager == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	AutoLocker<VirtualDirectoryManager> managerLocker(manager);
	BStringList directoryPaths;
	fStatus = manager->ResolveDirectoryPaths(*model->NodeRef(),
		*model->EntryRef(), directoryPaths, &fDefinitionFileRef);
	if (fStatus != B_OK)
		return;

	fStatus = _InitMergedDirectory(directoryPaths);
}


VirtualDirectoryEntryList::VirtualDirectoryEntryList(
	const node_ref& definitionFileRef, const BStringList& directoryPaths)
	:
	EntryListBase(),
	fDefinitionFileRef(definitionFileRef),
	fMergedDirectory(BMergedDirectory::B_ALWAYS_FIRST)
{
	fStatus = _InitMergedDirectory(directoryPaths);
}


VirtualDirectoryEntryList::~VirtualDirectoryEntryList()
{
}


status_t
VirtualDirectoryEntryList::InitCheck() const
{
	return EntryListBase::InitCheck();
}


status_t
VirtualDirectoryEntryList::GetNextEntry(BEntry* entry, bool traverse)
{
	entry_ref ref;
	status_t error = GetNextRef(&ref);
	if (error != B_OK)
		return error;

	return entry->SetTo(&ref, traverse);
}


status_t
VirtualDirectoryEntryList::GetNextRef(entry_ref* ref)
{
	BPrivate::Storage::LongDirEntry entry;
	int32 result = GetNextDirents(&entry, sizeof(entry), 1);
	if (result < 0)
		return result;
	if (result == 0)
		return B_ENTRY_NOT_FOUND;

	ref->device = entry.d_pdev;
	ref->directory = entry.d_pino;
	return ref->set_name(entry.d_name);
}


int32
VirtualDirectoryEntryList::GetNextDirents(struct dirent* buffer, size_t length,
	int32 count)
{
	if (count > 1)
		count = 1;

	int32 countRead = fMergedDirectory.GetNextDirents(buffer, length, count);
	if (countRead != 1)
		return countRead;

	// deal with directories
	entry_ref ref;
	ref.device = buffer->d_pdev;
	ref.directory = buffer->d_pino;
	if (ref.set_name(buffer->d_name) == B_OK && BEntry(&ref).IsDirectory()) {
		if (VirtualDirectoryManager* manager
				= VirtualDirectoryManager::Instance()) {
			AutoLocker<VirtualDirectoryManager> managerLocker(manager);
			manager->TranslateDirectoryEntry(fDefinitionFileRef, buffer);
		}
	}

	return countRead;
}


status_t
VirtualDirectoryEntryList::Rewind()
{
	return fMergedDirectory.Rewind();
}


int32
VirtualDirectoryEntryList::CountEntries()
{
	return 0;
}


status_t
VirtualDirectoryEntryList::_InitMergedDirectory(
	const BStringList& directoryPaths)
{
	status_t error = fMergedDirectory.Init();
	if (error != B_OK)
		return error;

	int32 count = directoryPaths.CountStrings();
	for (int32 i = 0; i < count; i++)
		fMergedDirectory.AddDirectory(directoryPaths.StringAt(i));

	return B_OK;
}

} // namespace BPrivate
