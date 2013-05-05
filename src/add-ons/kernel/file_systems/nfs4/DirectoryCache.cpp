/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "DirectoryCache.h"

#include <fs_cache.h>
#include <NodeMonitor.h>

#include "Inode.h"



NameCacheEntry::NameCacheEntry(const char* name, ino_t node)
	:
	fNode(node),
	fName(strdup(name))
{
	ASSERT(name != NULL);
}


NameCacheEntry::NameCacheEntry(const NameCacheEntry& entry)
	:
	fNode(entry.fNode),
	fName(strdup(entry.fName))
{
}


NameCacheEntry::~NameCacheEntry()
{
	free(const_cast<char*>(fName));
}


DirectoryCacheSnapshot::DirectoryCacheSnapshot()
{
	mutex_init(&fLock, NULL);
}


DirectoryCacheSnapshot::DirectoryCacheSnapshot(
	const DirectoryCacheSnapshot& snapshot)
{
	mutex_init(&fLock, NULL);

	MutexLocker _(snapshot.fLock);
	NameCacheEntry* entry = snapshot.fEntries.Head();
	NameCacheEntry* new_entry;
	while (entry) {
		new_entry = new NameCacheEntry(*entry);
		if (new_entry == NULL)
			break;

		fEntries.Add(new_entry);

		entry = snapshot.fEntries.GetNext(entry);
	}
}


DirectoryCacheSnapshot::~DirectoryCacheSnapshot()
{
	while (!fEntries.IsEmpty()) {
		NameCacheEntry* current = fEntries.RemoveHead();
		delete current;
	}

	mutex_destroy(&fLock);
}


DirectoryCache::DirectoryCache(Inode* inode, bool attr)
	:
	fExpirationTime(inode->fFileSystem->GetConfiguration().fDirectoryCacheTime),
	fDirectoryCache(NULL),
	fInode(inode),
	fAttrDir(attr),
	fTrashed(true)
{
	ASSERT(inode != NULL);

	mutex_init(&fLock, NULL);
}


DirectoryCache::~DirectoryCache()
{
	mutex_destroy(&fLock);
}


void
DirectoryCache::Reset()
{
	Trash();
	fExpireTime = system_time() + fExpirationTime;
	fTrashed = false;
}


void
DirectoryCache::Trash()
{
	while (!fNameCache.IsEmpty()) {
		NameCacheEntry* current = fNameCache.RemoveHead();
		entry_cache_remove(fInode->GetFileSystem()->DevId(), fInode->ID(),
			current->fName);
		delete current;
	}

	_SetSnapshot(NULL);

	fTrashed = true;
}


status_t
DirectoryCache::AddEntry(const char* name, ino_t node, bool created)
{
	ASSERT(name != NULL);

	NameCacheEntry* entry = new(std::nothrow) NameCacheEntry(name, node);
	if (entry == NULL)
		return B_NO_MEMORY;
	if (entry->fName == NULL) {
		delete entry;
		return B_NO_MEMORY;
	}

	fNameCache.Add(entry);

	if (created && fDirectoryCache != NULL) {
		MutexLocker _(fDirectoryCache->fLock);
		NameCacheEntry* entry = new(std::nothrow) NameCacheEntry(name, node);
		if (entry == NULL)
			return B_NO_MEMORY;
		if (entry->fName == NULL) {
			delete entry;
			return B_NO_MEMORY;
		}

		fDirectoryCache->fEntries.Add(entry);
	}

	if (!fAttrDir) {
		return entry_cache_add(fInode->GetFileSystem()->DevId(), fInode->ID(),
			name, node);
	}

	return B_OK;
}


void
DirectoryCache::RemoveEntry(const char* name)
{
	ASSERT(name != NULL);

	SinglyLinkedList<NameCacheEntry>::Iterator iterator
		= fNameCache.GetIterator();
	NameCacheEntry* previous = NULL;
	NameCacheEntry* current = iterator.Next();
	while (current != NULL) {
		if (strcmp(current->fName, name) == 0) {
			fNameCache.Remove(previous, current);
			delete current;
			break;
		}

		previous = current;
		current = iterator.Next();
	}

	if (fDirectoryCache != NULL) {
		MutexLocker _(fDirectoryCache->fLock);
		iterator = fDirectoryCache->fEntries.GetIterator();
		previous = NULL;
		current = iterator.Next();
		while (current != NULL) {
			if (strcmp(current->fName, name) == 0) {
				fDirectoryCache->fEntries.Remove(previous, current);
				delete current;
				break;
			}

			previous = current;
			current = iterator.Next();
		}
	}

	if (!fAttrDir) {
		entry_cache_remove(fInode->GetFileSystem()->DevId(), fInode->ID(),
			name);
	}
}


void
DirectoryCache::_SetSnapshot(DirectoryCacheSnapshot* snapshot)
{
	if (fDirectoryCache != NULL)
		fDirectoryCache->ReleaseReference();
	fDirectoryCache = snapshot;
}


status_t
DirectoryCache::_LoadSnapshot(bool trash)
{
	DirectoryCacheSnapshot* oldSnapshot = fDirectoryCache;
	if (oldSnapshot != NULL)
		oldSnapshot->AcquireReference();

	if (trash)
		Trash();

	DirectoryCacheSnapshot* newSnapshot;
	status_t result = fInode->GetDirSnapshot(&newSnapshot, NULL, &fChange,
		fAttrDir);
	if (result != B_OK) {
		if (oldSnapshot != NULL)
			oldSnapshot->ReleaseReference();
		return result;
	}
	newSnapshot->AcquireReference();

	_SetSnapshot(newSnapshot);
	fExpireTime = system_time() + fExpirationTime;

	fTrashed = false;

	if (oldSnapshot != NULL)
		NotifyChanges(oldSnapshot, newSnapshot);

	if (oldSnapshot != NULL)
		oldSnapshot->ReleaseReference();

	newSnapshot->ReleaseReference();
	return B_OK;
}


status_t
DirectoryCache::Revalidate()
{
	if (fExpireTime > system_time())
		return B_OK;

	uint64 change;
	if (fInode->GetChangeInfo(&change, fAttrDir) != B_OK) {
		Trash();
		return B_ERROR;
	}

	if (change == fChange) {
		fExpireTime = system_time() + fExpirationTime;
		return B_OK;
	}

	return _LoadSnapshot(true);
}


void
DirectoryCache::NotifyChanges(DirectoryCacheSnapshot* oldSnapshot,
	DirectoryCacheSnapshot* newSnapshot)
{
	ASSERT(newSnapshot != NULL);
	ASSERT(oldSnapshot != NULL);

	MutexLocker _(newSnapshot->fLock);

	SinglyLinkedList<NameCacheEntry>::Iterator oldIt
		= oldSnapshot->fEntries.GetIterator();
	NameCacheEntry* oldCurrent;

	SinglyLinkedList<NameCacheEntry>::Iterator newIt
		= newSnapshot->fEntries.GetIterator();
	NameCacheEntry* newCurrent = newIt.Next();
	while (newCurrent != NULL) {
		oldIt = oldSnapshot->fEntries.GetIterator();
		oldCurrent = oldIt.Next();

		bool found = false;
		NameCacheEntry* prev = NULL;
		while (oldCurrent != NULL) {
			if (oldCurrent->fNode == newCurrent->fNode
				&& strcmp(oldCurrent->fName, newCurrent->fName) == 0) {
				found = true;
				break;
			}

			prev = oldCurrent;
			oldCurrent = oldIt.Next();
		}

		if (!found) {
			if (fAttrDir) {
				notify_attribute_changed(fInode->GetFileSystem()->DevId(),
					fInode->ID(), newCurrent->fName, B_ATTR_CREATED);
			} else {
				notify_entry_created(fInode->GetFileSystem()->DevId(),
					fInode->ID(), newCurrent->fName, newCurrent->fNode);
			}
		} else
			oldSnapshot->fEntries.Remove(prev, oldCurrent);

		newCurrent = newIt.Next();
	}

	oldIt = oldSnapshot->fEntries.GetIterator();
	oldCurrent = oldIt.Next();

	while (oldCurrent != NULL) {
		if (fAttrDir) {
			notify_attribute_changed(fInode->GetFileSystem()->DevId(),
				fInode->ID(), oldCurrent->fName, B_ATTR_REMOVED);
		} else {
			notify_entry_removed(fInode->GetFileSystem()->DevId(), fInode->ID(),
				oldCurrent->fName, oldCurrent->fNode);
		}
		oldCurrent = oldIt.Next();
	}
}

