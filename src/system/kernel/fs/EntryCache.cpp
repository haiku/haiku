/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EntryCache.h"

#include <new>
#include <vm/vm.h>
#include <slab/Slab.h>


static const int32 kEntryNotInArray = -1;
static const int32 kEntryRemoved = -2;


// #pragma mark - EntryCacheGeneration


EntryCacheGeneration::EntryCacheGeneration()
	:
	next_index(0),
	entries(NULL)
{
}


EntryCacheGeneration::~EntryCacheGeneration()
{
	delete[] entries;
}


status_t
EntryCacheGeneration::Init(int32 entriesSize)
{
	entries_size = entriesSize;
	entries = new(std::nothrow) EntryCacheEntry*[entries_size];
	if (entries == NULL)
		return B_NO_MEMORY;

	memset(entries, 0, sizeof(EntryCacheEntry*) * entries_size);
	return B_OK;
}


// #pragma mark - EntryCache


EntryCache::EntryCache()
	:
	fGenerationCount(0),
	fGenerations(NULL),
	fCurrentGeneration(0)
{
	rw_lock_init(&fLock, "entry cache");

	new(&fEntries) EntryTable;
}


EntryCache::~EntryCache()
{
	// delete entries
	EntryCacheEntry* entry = fEntries.Clear(true);
	while (entry != NULL) {
		EntryCacheEntry* next = entry->hash_link;
		free(entry);
		entry = next;
	}
	delete[] fGenerations;

	rw_lock_destroy(&fLock);
}


status_t
EntryCache::Init()
{
	status_t error = fEntries.Init();
	if (error != B_OK)
		return error;

	int32 entriesSize = 1024;
	fGenerationCount = 8;

	// TODO: Choose generation size/count more scientifically?
	// TODO: Add low_resource handler hook?
	if (vm_available_memory() >= (1024*1024*1024)) {
		entriesSize = 8192;
		fGenerationCount = 16;
	}

	fGenerations = new(std::nothrow) EntryCacheGeneration[fGenerationCount];
	for (int32 i = 0; i < fGenerationCount; i++) {
		error = fGenerations[i].Init(entriesSize);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
EntryCache::Add(ino_t dirID, const char* name, ino_t nodeID, bool missing)
{
	EntryCacheKey key(dirID, name);

	const size_t nameLen = strlen(name);
	EntryCacheEntry* entry = (EntryCacheEntry*)malloc(sizeof(EntryCacheEntry) + nameLen);

	if (entry == NULL)
		return B_NO_MEMORY;

	entry->node_id = nodeID;
	entry->dir_id = dirID;
	entry->hash = key.hash;
	entry->missing = missing;
	entry->index = kEntryNotInArray;
	entry->generation = -1;
	memcpy(entry->name, name, nameLen + 1);

	ReadLocker readLocker(fLock);

	if (fGenerationCount == 0) {
		free(entry);
		return B_NO_MEMORY;
	}

	EntryCacheEntry* existingEntry = fEntries.InsertAtomic(entry);
	if (existingEntry != NULL) {
		free(entry);
		entry = existingEntry;

		entry->node_id = nodeID;
		entry->missing = missing;
	}

	readLocker.Detach();
	_AddEntryToCurrentGeneration(entry, entry == existingEntry);
	return B_OK;
}


status_t
EntryCache::Remove(ino_t dirID, const char* name)
{
	EntryCacheKey key(dirID, name);

	WriteLocker writeLocker(fLock);

	EntryCacheEntry* entry = fEntries.Lookup(key);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	fEntries.Remove(entry);

	if (entry->index >= 0) {
		// remove the entry from its generation and delete it
		fGenerations[entry->generation].entries[entry->index] = NULL;
		writeLocker.Unlock();
		free(entry);
	} else {
		// We can't free it, since another thread is about to try to move it
		// to another generation. We mark it removed and the other thread will
		// take care of deleting it.
		entry->index = kEntryRemoved;
	}

	return B_OK;
}


bool
EntryCache::Lookup(ino_t dirID, const char* name, ino_t& _nodeID,
	bool& _missing)
{
	EntryCacheKey key(dirID, name);

	ReadLocker readLocker(fLock);

	EntryCacheEntry* entry = fEntries.Lookup(key);
	if (entry == NULL)
		return false;

	_nodeID = entry->node_id;
	_missing = entry->missing;

	readLocker.Detach();
	return _AddEntryToCurrentGeneration(entry, true);
}


const char*
EntryCache::DebugReverseLookup(ino_t nodeID, ino_t& _dirID)
{
	for (EntryTable::Iterator it = fEntries.GetIterator();
			EntryCacheEntry* entry = it.Next();) {
		if (nodeID == entry->node_id && strcmp(entry->name, ".") != 0
				&& strcmp(entry->name, "..") != 0) {
			_dirID = entry->dir_id;
			return entry->name;
		}
	}

	return NULL;
}


bool
EntryCache::_AddEntryToCurrentGeneration(EntryCacheEntry* entry, bool move)
{
	ReadLocker readLocker(fLock, true);

	if (move) {
		const int32 oldGeneration = atomic_get_and_set(&entry->generation,
			fCurrentGeneration);
		if (oldGeneration == fCurrentGeneration || entry->index < 0) {
			// The entry is already in the current generation or is being moved to
			// it by another thread.
			return true;
		}

		// remove from old generation array
		fGenerations[oldGeneration].entries[entry->index] = NULL;
		entry->index = kEntryNotInArray;
	} else {
		entry->generation = fCurrentGeneration;
	}

	// add to the current generation
	int32 index = atomic_add(&fGenerations[fCurrentGeneration].next_index, 1);
	if (index < fGenerations[fCurrentGeneration].entries_size) {
		fGenerations[fCurrentGeneration].entries[index] = entry;
		entry->index = index;
		return true;
	}

	// The current generation is full, so we probably need to clear the oldest
	// one to make room. We need the write lock for that.
	readLocker.Unlock();
	WriteLocker writeLocker(fLock);

	if (entry->index == kEntryRemoved) {
		// the entry has been removed in the meantime
		writeLocker.Unlock();
		free(entry);
		return false;
	}

	// the generation might not be full yet
	index = fGenerations[fCurrentGeneration].next_index++;
	if (index < fGenerations[fCurrentGeneration].entries_size) {
		fGenerations[fCurrentGeneration].entries[index] = entry;
		entry->generation = fCurrentGeneration;
		entry->index = index;

		fEntries.ResizeIfNeeded();
		return true;
	}

	// we have to clear the oldest generation
	EntryCacheEntry* entriesToFree = NULL;
	const int32 newGeneration = (fCurrentGeneration + 1) % fGenerationCount;
	for (int32 i = 0; i < fGenerations[newGeneration].entries_size; i++) {
		EntryCacheEntry* otherEntry = fGenerations[newGeneration].entries[i];
		if (otherEntry == NULL)
			continue;

		fGenerations[newGeneration].entries[i] = NULL;
		fEntries.RemoveUnchecked(otherEntry);

		otherEntry->hash_link = entriesToFree;
		entriesToFree = otherEntry;
	}

	// set the new generation and add the entry
	fCurrentGeneration = newGeneration;
	fGenerations[newGeneration].entries[0] = entry;
	fGenerations[newGeneration].next_index = 1;
	entry->generation = newGeneration;
	entry->index = 0;

	// free the old entries
	writeLocker.Unlock();
	while (entriesToFree != NULL) {
		EntryCacheEntry* next = entriesToFree->hash_link;
		free(entriesToFree);
		entriesToFree = next;
	}

	return true;
}
