/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EntryCache.h"

#include <new>


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

	fGenerationCount = 8;
	int32 entriesSize = 1024;

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

	WriteLocker _(fLock);

	if (fGenerationCount == 0)
		return B_NO_MEMORY;

	EntryCacheEntry* entry = fEntries.Lookup(key);
	if (entry != NULL) {
		entry->node_id = nodeID;
		entry->missing = missing;
		if (entry->generation != fCurrentGeneration) {
			if (entry->index >= 0) {
				fGenerations[entry->generation].entries[entry->index] = NULL;
				_AddEntryToCurrentGeneration(entry);
			}
		}
		return B_OK;
	}

	entry = (EntryCacheEntry*)malloc(sizeof(EntryCacheEntry) + strlen(name));
	if (entry == NULL)
		return B_NO_MEMORY;

	entry->node_id = nodeID;
	entry->dir_id = dirID;
	entry->missing = missing;
	entry->generation = fCurrentGeneration;
	entry->index = kEntryNotInArray;
	strcpy(entry->name, name);

	fEntries.Insert(entry);

	_AddEntryToCurrentGeneration(entry);

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

	const int32 oldGeneration = atomic_get_and_set(&entry->generation,
		fCurrentGeneration);
	if (oldGeneration == fCurrentGeneration || entry->index < 0) {
		// The entry is already in the current generation or is being moved to
		// it by another thread.
		_nodeID = entry->node_id;
		_missing = entry->missing;
		return true;
	}

	// remove from old generation array
	fGenerations[oldGeneration].entries[entry->index] = NULL;
	entry->index = kEntryNotInArray;

	// add to the current generation
	const int32 index = atomic_add(&fGenerations[fCurrentGeneration].next_index, 1);
	if (index < fGenerations[fCurrentGeneration].entries_size) {
		fGenerations[fCurrentGeneration].entries[index] = entry;
		entry->index = index;
		_nodeID = entry->node_id;
		_missing = entry->missing;
		return true;
	}

	// The current generation is full, so we probably need to clear the oldest
	// one to make room. We need the write lock for that.
	readLocker.Unlock();
	WriteLocker writeLocker(fLock);

	if (entry->index == kEntryRemoved) {
		// the entry has been removed in the meantime
		free(entry);
		return false;
	}

	_AddEntryToCurrentGeneration(entry);

	_nodeID = entry->node_id;
	_missing = entry->missing;
	return true;
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


void
EntryCache::_AddEntryToCurrentGeneration(EntryCacheEntry* entry)
{
	ASSERT_WRITE_LOCKED_RW_LOCK(&fLock);

	// the generation might not be full yet
	int32 index = fGenerations[fCurrentGeneration].next_index++;
	if (index < fGenerations[fCurrentGeneration].entries_size) {
		fGenerations[fCurrentGeneration].entries[index] = entry;
		entry->generation = fCurrentGeneration;
		entry->index = index;
		return;
	}

	// we have to clear the oldest generation
	const int32 newGeneration = (fCurrentGeneration + 1) % fGenerationCount;
	for (int32 i = 0; i < fGenerations[newGeneration].entries_size; i++) {
		EntryCacheEntry* otherEntry = fGenerations[newGeneration].entries[i];
		if (otherEntry == NULL)
			continue;

		fGenerations[newGeneration].entries[i] = NULL;
		fEntries.Remove(otherEntry);
		free(otherEntry);
	}

	// set the new generation and add the entry
	fCurrentGeneration = newGeneration;
	fGenerations[newGeneration].entries[0] = entry;
	fGenerations[newGeneration].next_index = 1;
	entry->generation = newGeneration;
	entry->index = 0;
}
