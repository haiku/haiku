/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "DirectoryCache.h"

#include <fs_cache.h>

#include "Inode.h"



NameCacheEntry::NameCacheEntry(const char* name, ino_t node)
	:
	fNode(node),
	fName(strdup(name))
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


DirectoryCacheSnapshot::~DirectoryCacheSnapshot()
{
	while (!fEntries.IsEmpty()) {
		NameCacheEntry* current = fEntries.RemoveHead();
		delete current;
	}

	mutex_destroy(&fLock);
}


DirectoryCache::DirectoryCache(Inode* inode)
	:
	fDirectoryCache(NULL),
	fInode(inode),
	fTrashed(true)
{
	mutex_init(&fLock, NULL);
}


DirectoryCache::~DirectoryCache()
{
	mutex_destroy(&fLock);
}


void
DirectoryCache::ResetAndLock()
{
	mutex_lock(&fLock);
	Trash();
	fExpireTime = system_time() + kExpirationTime;
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

	SetSnapshot(NULL);

	fTrashed = true;
}


status_t
DirectoryCache::AddEntry(const char* name, ino_t node, bool created)
{
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

	return entry_cache_add(fInode->GetFileSystem()->DevId(), fInode->ID(), name,
		node);
}

void
DirectoryCache::RemoveEntry(const char* name)
{
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

	entry_cache_remove(fInode->GetFileSystem()->DevId(), fInode->ID(), name);
}


void
DirectoryCache::SetSnapshot(DirectoryCacheSnapshot* snapshot)
{
	if (fDirectoryCache != NULL)
		fDirectoryCache->ReleaseReference();
	fDirectoryCache = snapshot;
}


status_t
DirectoryCache::Revalidate()
{
	uint64 change;
	if (fInode->GetChangeInfo(&change) == B_OK && change == fChange) {
		fExpireTime = system_time() + kExpirationTime;
		return B_OK;
	}

	Trash();
	return B_ERROR;
}

