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


DirectoryCache::DirectoryCache(Inode* inode)
	:
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
		free(const_cast<char*>(current->fName));
		delete current;
	}

	fTrashed = true;
}


status_t
DirectoryCache::AddEntry(const char* name, ino_t node)
{
	NameCacheEntry* entry = new(std::nothrow) NameCacheEntry;
	if (entry == NULL)
		return B_NO_MEMORY;

	entry->fName = strdup(name);
	if (entry->fName == NULL) {
		delete entry;
		return B_NO_MEMORY;
	}
	entry->fNode = node;
	fNameCache.Add(entry);

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
			free(const_cast<char*>(current->fName));
			delete current;

			fNameCache.Remove(previous, current);
			break;
		}

		previous = current;
		current = iterator.Next();
	}

	entry_cache_remove(fInode->GetFileSystem()->DevId(), fInode->ID(), name);
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

