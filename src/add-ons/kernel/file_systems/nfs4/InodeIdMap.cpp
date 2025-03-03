/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "InodeIdMap.h"


status_t
InodeIdMap::AddName(FileInfo& fileInfo, InodeNames* parent,
	const char* name, ino_t id)
{
	MutexLocker _(fLock);
	AVLTreeMap<ino_t, FileInfo>::Iterator iterator = fMap.Find(id);
	if (iterator.HasCurrent()) {
		if (fileInfo.fHandle == iterator.Current().fHandle) {
			return iterator.CurrentValuePointer()->fNames->AddName(parent,
				name);
		}
	}

	fMap.Remove(id);
	fileInfo.fNames = new InodeNames;
	if (fileInfo.fNames == NULL)
		return B_NO_MEMORY;

	fileInfo.fNames->fHandle = fileInfo.fHandle;
	status_t result = fileInfo.fNames->AddName(parent, name);
	if (result != B_OK) {
		delete fileInfo.fNames;
		return result;
	}

	return fMap.Insert(id, fileInfo);
}


bool
InodeIdMap::RemoveName(ino_t id, InodeNames* parent, const char* name)
{
	ASSERT(name != NULL);

	MutexLocker _(fLock);
	AVLTreeMap<ino_t, FileInfo>::Iterator iterator = fMap.Find(id);
	if (!iterator.HasCurrent())
		return true;

	FileInfo* fileInfo = iterator.CurrentValuePointer();

	return fileInfo->fNames->RemoveName(parent, name);
}


status_t
InodeIdMap::RemoveEntry(ino_t id)
{
	MutexLocker _(fLock);
	return fMap.Remove(id);
}


status_t
InodeIdMap::GetFileInfo(FileInfo* fileInfo, ino_t id)
{
	ASSERT(fileInfo != NULL);

	MutexLocker _(fLock);
	AVLTreeMap<ino_t, FileInfo>::Iterator iterator = fMap.Find(id);
	if (!iterator.HasCurrent())
		return B_ENTRY_NOT_FOUND;

	*fileInfo = iterator.Current();
	if (fileInfo->fNames->fNames.IsEmpty())
		return B_ENTRY_NOT_FOUND;
	return B_OK;
}


void
InodeIdMap::Dump(void (*xprintf)(const char*, ...))
{
	MutexLocker locker;
	if (xprintf != kprintf)
		locker.SetTo(fLock, false);

	_DumpLocked(xprintf);

	return;
}


void
InodeIdMap::_DumpLocked(void (*xprintf)(const char*, ...)) const
{
	xprintf("InodeIdMap at %p\n", this);
	xprintf("\tino\thandle\n");
	AVLTreeMap<ino_t, FileInfo>::ConstIterator iterator = fMap.GetIterator();
	for (iterator.Next(); iterator.HasNext(); iterator.Next()) {
		ino_t ino = iterator.CurrentKey();
		xprintf("\t%" B_PRIdINO "\t", ino);
		const FileInfo* fileInfo = iterator.CurrentValuePointer();
		if (fileInfo != NULL)
			fileInfo->fHandle.Dump(xprintf);
	}

	return;
}

